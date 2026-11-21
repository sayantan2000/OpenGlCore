#include "Physics.h"
#include "Scene.h"
#include "GameObject.h"
#include "Model.h"
#include "Mesh.h"

#include <PxPhysicsAPI.h>
#include <characterkinematic/PxControllerManager.h>
#include <characterkinematic/PxCapsuleController.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <string>
#include <iostream>
#include <cmath>

namespace ogl {

using namespace physx;

namespace
{
	// Collision filter that enables touch (contact) and trigger notifications.
	PxFilterFlags FilterShader(
		PxFilterObjectAttributes a0, PxFilterData,
		PxFilterObjectAttributes a1, PxFilterData,
		PxPairFlags& pairFlags, const void*, PxU32)
	{
		if (PxFilterObjectIsTrigger(a0) || PxFilterObjectIsTrigger(a1))
		{
			pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
			return PxFilterFlag::eDEFAULT;
		}
		pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_LOST;
		return PxFilterFlag::eDEFAULT;
	}

	std::uint32_t IdOf(const PxActor* a) { return a ? (std::uint32_t)(std::uintptr_t)a->userData : 0u; }

	// Extract translation + rotation (scale stripped) from a world matrix.
	void DecomposeTR(const glm::mat4& m, PxVec3& pos, PxQuat& rot, glm::vec3& scale)
	{
		pos = PxVec3(m[3].x, m[3].y, m[3].z);
		glm::vec3 c0(m[0]), c1(m[1]), c2(m[2]);
		scale = glm::vec3(glm::length(c0), glm::length(c1), glm::length(c2));
		glm::mat3 r(c0 / glm::max(scale.x, 1e-6f), c1 / glm::max(scale.y, 1e-6f), c2 / glm::max(scale.z, 1e-6f));
		glm::quat q = glm::normalize(glm::quat_cast(r));
		rot = PxQuat(q.x, q.y, q.z, q.w);
	}
}

// Collects contact/trigger pairs into the scene's event list.
struct PhysicsScene::Impl : public PxSimulationEventCallback
{
	PxDefaultAllocator alloc;
	PxDefaultErrorCallback errCb;
	PxFoundation* foundation = nullptr;
	PxPhysics* physics = nullptr;
	PxDefaultCpuDispatcher* dispatcher = nullptr;
	PxScene* scene = nullptr;
	PxControllerManager* controllerMgr = nullptr;
	PxMaterial* controllerMat = nullptr;

	std::unordered_map<std::uint32_t, PxRigidActor*> actors; // GameObject id -> actor
	// Character controllers keyed by GameObject id, with cached grounded state from the last move.
	struct CharacterRuntime { PxController* controller = nullptr; float vertVel = 0.0f; bool grounded = false; };
	std::unordered_map<std::uint32_t, CharacterRuntime> characters;
	// Cooked mesh caches, keyed by the renderer's sourceId so duplicate objects share one cook.
	std::unordered_map<std::string, PxConvexMesh*>   convexCache;
	std::unordered_map<std::string, PxTriangleMesh*> triangleCache;
	std::vector<ContactEvent> events;
	glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
	float fixedDt = 1.0f / 60.0f;
	float accumulator = 0.0f;
	bool running = false;

	// PxSimulationEventCallback
	void onContact(const PxContactPairHeader& h, const PxContactPair* pairs, PxU32 n) override
	{
		std::uint32_t a = IdOf(h.actors[0]), b = IdOf(h.actors[1]);
		for (PxU32 i = 0; i < n; ++i)
		{
			if (pairs[i].events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
				events.push_back({ ContactEvent::Kind::CollisionEnter, a, b });
			else if (pairs[i].events & PxPairFlag::eNOTIFY_TOUCH_LOST)
				events.push_back({ ContactEvent::Kind::CollisionExit, a, b });
		}
	}
	void onTrigger(PxTriggerPair* pairs, PxU32 n) override
	{
		for (PxU32 i = 0; i < n; ++i)
		{
			if (pairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER)) continue;
			std::uint32_t t = IdOf(pairs[i].triggerActor), o = IdOf(pairs[i].otherActor);
			if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
				events.push_back({ ContactEvent::Kind::TriggerEnter, t, o });
			else if (pairs[i].status & PxPairFlag::eNOTIFY_TOUCH_LOST)
				events.push_back({ ContactEvent::Kind::TriggerExit, t, o });
		}
	}
	void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
	void onWake(PxActor**, PxU32) override {}
	void onSleep(PxActor**, PxU32) override {}
	void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}

	bool Init()
	{
		foundation = PxCreateFoundation(PX_PHYSICS_VERSION, alloc, errCb);
		if (!foundation) return false;
		physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, nullptr);
		if (!physics) return false;
		dispatcher = PxDefaultCpuDispatcherCreate(2);
		return true;
	}

	void Shutdown()
	{
		End();
		for (auto& [k, m] : convexCache)   if (m) m->release();
		for (auto& [k, m] : triangleCache) if (m) m->release();
		convexCache.clear();
		triangleCache.clear();
		if (dispatcher) { dispatcher->release(); dispatcher = nullptr; }
		if (physics) { physics->release(); physics = nullptr; }
		if (foundation) { foundation->release(); foundation = nullptr; }
	}

	void BuildScene(PxScene*& out)
	{
		PxSceneDesc desc(physics->getTolerancesScale());
		desc.gravity = PxVec3(gravity.x, gravity.y, gravity.z);
		desc.cpuDispatcher = dispatcher;
		desc.filterShader = FilterShader;
		desc.simulationEventCallback = this;
		out = physics->createScene(desc);
	}

	// Pulls CPU collision geometry (object-space positions + indices) from a GameObject's
	// mesh renderer. Returns false when the object has no usable geometry.
	static bool GetGeometry(const GameObject* go, const glm::vec3*& pts, PxU32& nPts,
		size_t ptStride, const unsigned int*& idx, PxU32& nIdx)
	{
		if (!go || !go->meshRenderer) return false;
		const MeshRenderer& mr = *go->meshRenderer;
		if (mr.kind == MeshRenderer::Geometry::Model && mr.model)
		{
			const auto& p = mr.model->CollisionPositions();
			const auto& i = mr.model->CollisionIndices();
			if (p.empty() || i.empty()) return false;
			pts = p.data(); nPts = (PxU32)p.size(); idx = i.data(); nIdx = (PxU32)i.size();
			return true;
		}
		if (mr.kind == MeshRenderer::Geometry::Mesh && mr.mesh)
		{
			const auto& p = mr.mesh->Positions();
			const auto& i = mr.mesh->Indices();
			if (p.empty() || i.empty()) return false;
			pts = p.data(); nPts = (PxU32)p.size(); idx = i.data(); nIdx = (PxU32)i.size();
			return true;
		}
		return false;
	}

	PxConvexMesh* CookConvex(const glm::vec3* pts, PxU32 nPts)
	{
		PxConvexMeshDesc desc;
		desc.points.count = nPts;
		desc.points.stride = sizeof(glm::vec3);
		desc.points.data = pts;
		desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
		desc.vertexLimit = 255;
		PxCookingParams params(physics->getTolerancesScale());
		PxConvexMeshCookingResult::Enum result;
		PxConvexMesh* mesh = PxCreateConvexMesh(params, desc, physics->getPhysicsInsertionCallback(), &result);
		if (!mesh)
			std::cerr << "[Physics] Convex cook failed (result " << (int)result << ")." << std::endl;
		return mesh;
	}

	PxTriangleMesh* CookTriangle(const glm::vec3* pts, PxU32 nPts, const unsigned int* idx, PxU32 nIdx)
	{
		PxTriangleMeshDesc desc;
		desc.points.count = nPts;
		desc.points.stride = sizeof(glm::vec3);
		desc.points.data = pts;
		desc.triangles.count = nIdx / 3;
		desc.triangles.stride = 3 * sizeof(unsigned int);
		desc.triangles.data = idx;
		PxCookingParams params(physics->getTolerancesScale());
		PxTriangleMeshCookingResult::Enum result;
		PxTriangleMesh* mesh = PxCreateTriangleMesh(params, desc, physics->getPhysicsInsertionCallback(), &result);
		if (!mesh)
			std::cerr << "[Physics] Triangle cook failed (result " << (int)result << ")." << std::endl;
		return mesh;
	}

	// `dynamicSim` is true for a non-kinematic dynamic actor: PhysX cannot simulate a triangle
	// mesh on such a body, so a TriangleMesh request is cooked as a convex hull instead.
	PxShape* MakeShape(const GameObject* go, const glm::vec3& scale, bool dynamicSim)
	{
		const ColliderComponent& col = *go->collider;
		PxMaterial* mat = physics->createMaterial(col.material.staticFriction, col.material.dynamicFriction, col.material.restitution);
		float sx = scale.x, sy = scale.y, sz = scale.z;
		float avg = (sx + sy + sz) / 3.0f;
		PxShape* shape = nullptr;
		switch (col.shape)
		{
		case ColliderComponent::Shape::Box:
			shape = physics->createShape(PxBoxGeometry(col.halfExtents.x * sx, col.halfExtents.y * sy, col.halfExtents.z * sz), *mat, true);
			break;
		case ColliderComponent::Shape::Sphere:
			shape = physics->createShape(PxSphereGeometry(col.radius * avg), *mat, true);
			break;
		case ColliderComponent::Shape::Capsule:
			shape = physics->createShape(PxCapsuleGeometry(col.radius * glm::max(sx, sz), col.halfHeight * sy), *mat, true);
			break;
		case ColliderComponent::Shape::Plane:
			shape = physics->createShape(PxBoxGeometry(50.0f, 0.02f, 50.0f), *mat, true); // finite "ground"
			break;
		case ColliderComponent::Shape::ConvexMesh:
		case ColliderComponent::Shape::TriangleMesh:
		{
			const glm::vec3* pts = nullptr; const unsigned int* idx = nullptr;
			PxU32 nPts = 0, nIdx = 0;
			if (!GetGeometry(go, pts, nPts, sizeof(glm::vec3), idx, nIdx))
			{
				std::cerr << "[Physics] Mesh collider on '" << go->name
					<< "' has no geometry; using a box approximation." << std::endl;
				shape = physics->createShape(PxBoxGeometry(col.halfExtents.x * sx, col.halfExtents.y * sy, col.halfExtents.z * sz), *mat, true);
				break;
			}
			const std::string& key = go->meshRenderer->sourceId;
			PxMeshScale meshScale(PxVec3(sx, sy, sz));
			// Triangle meshes can only back static/kinematic actors; downgrade dynamic ones.
			bool wantTriangle = (col.shape == ColliderComponent::Shape::TriangleMesh) && !dynamicSim;
			if (col.shape == ColliderComponent::Shape::TriangleMesh && dynamicSim)
				std::cerr << "[Physics] Triangle-mesh collider on dynamic '" << go->name
					<< "' is unsupported; cooking a convex hull instead." << std::endl;

			if (wantTriangle)
			{
				PxTriangleMesh*& cached = triangleCache[key];
				if (!cached) cached = CookTriangle(pts, nPts, idx, nIdx);
				if (cached) shape = physics->createShape(PxTriangleMeshGeometry(cached, meshScale), *mat, true);
			}
			else
			{
				PxConvexMesh*& cached = convexCache[key];
				if (!cached) cached = CookConvex(pts, nPts);
				if (cached) shape = physics->createShape(PxConvexMeshGeometry(cached, meshScale), *mat, true);
			}
			if (!shape) // cook failed -> safe fallback
				shape = physics->createShape(PxBoxGeometry(col.halfExtents.x * sx, col.halfExtents.y * sy, col.halfExtents.z * sz), *mat, true);
			break;
		}
		}
		if (shape)
		{
			shape->setLocalPose(PxTransform(PxVec3(col.center.x, col.center.y, col.center.z)));
			if (col.isTrigger)
			{
				shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
				shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
			}
		}
		mat->release(); // shape holds a reference
		return shape;
	}

	void Begin(Scene& gameScene)
	{
		if (!physics) return;
		End();
		BuildScene(scene);

		for (auto& up : gameScene.Objects())
		{
			GameObject* go = up.get();
			if (!go->active || !go->collider || !go->collider->enabled) continue;

			PxVec3 pos; PxQuat rot; glm::vec3 scale;
			DecomposeTR(gameScene.WorldMatrix(go), pos, rot, scale);
			PxTransform pose(pos, rot);

			bool hasBody = go->rigidBody && go->rigidBody->enabled;
			RigidBodyComponent::BodyType type = hasBody ? go->rigidBody->type : RigidBodyComponent::BodyType::Static;
			bool dynamicSim = (type == RigidBodyComponent::BodyType::Dynamic);

			PxShape* shape = MakeShape(go, scale, dynamicSim);
			if (!shape) continue;

			PxRigidActor* actor = nullptr;
			if (type == RigidBodyComponent::BodyType::Static)
			{
				PxRigidStatic* s = physics->createRigidStatic(pose);
				s->attachShape(*shape);
				actor = s;
			}
			else
			{
				PxRigidDynamic* d = physics->createRigidDynamic(pose);
				d->attachShape(*shape);
				const RigidBodyComponent& rb = *go->rigidBody;
				PxRigidBodyExt::updateMassAndInertia(*d, glm::max(rb.mass, 0.001f) /* density-ish */);
				d->setMass(glm::max(rb.mass, 0.001f));
				d->setLinearDamping(rb.linearDamping);
				d->setAngularDamping(rb.angularDamping);
				d->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !rb.useGravity);
				if (type == RigidBodyComponent::BodyType::Kinematic)
					d->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
				else
				{
					d->setLinearVelocity(PxVec3(rb.initialLinearVelocity.x, rb.initialLinearVelocity.y, rb.initialLinearVelocity.z));
					d->setAngularVelocity(PxVec3(rb.initialAngularVelocity.x, rb.initialAngularVelocity.y, rb.initialAngularVelocity.z));
				}
				PxRigidDynamicLockFlags lock = PxRigidDynamicLockFlags(0);
				if (rb.freezePosX) lock |= PxRigidDynamicLockFlag::eLOCK_LINEAR_X;
				if (rb.freezePosY) lock |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Y;
				if (rb.freezePosZ) lock |= PxRigidDynamicLockFlag::eLOCK_LINEAR_Z;
				if (rb.freezeRotX) lock |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_X;
				if (rb.freezeRotY) lock |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y;
				if (rb.freezeRotZ) lock |= PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z;
				d->setRigidDynamicLockFlags(lock);
				actor = d;
			}
			shape->release(); // actor holds a reference
			actor->userData = (void*)(std::uintptr_t)go->id;
			scene->addActor(*actor);
			actors[go->id] = actor;
		}

		// Second pass: create character controllers (independent of the collider list above).
		if (!controllerMgr) controllerMgr = PxCreateControllerManager(*scene);
		if (controllerMgr)
		{
			if (!controllerMat) controllerMat = physics->createMaterial(0.5f, 0.5f, 0.0f);
			for (auto& up : gameScene.Objects())
			{
				GameObject* go = up.get();
				if (!go->active || !go->characterController || !go->characterController->enabled) continue;
				const CharacterControllerComponent& cc = *go->characterController;

				glm::mat4 world = gameScene.WorldMatrix(go);
				glm::vec3 wpos = glm::vec3(world[3]) + cc.center;

				PxCapsuleControllerDesc desc;
				desc.height = glm::max(cc.height, 0.01f);
				desc.radius = glm::max(cc.radius, 0.01f);
				desc.climbingMode = PxCapsuleClimbingMode::eEASY;
				desc.slopeLimit = cosf(glm::radians(cc.slopeLimitDeg));
				desc.stepOffset = cc.stepOffset;
				desc.contactOffset = glm::max(cc.skinWidth, 0.001f);
				desc.upDirection = PxVec3(0, 1, 0);
				desc.material = controllerMat;
				// PxController origin is the capsule centre; place it so the object pivot sits at wpos.
				desc.position = PxExtendedVec3(wpos.x, wpos.y, wpos.z);
				desc.reportCallback = nullptr;
				if (!desc.isValid()) { std::cerr << "[Physics] Invalid character controller on '" << go->name << "'." << std::endl; continue; }

				PxController* ctrl = controllerMgr->createController(desc);
				if (!ctrl) continue;
				if (ctrl->getActor()) ctrl->getActor()->userData = (void*)(std::uintptr_t)go->id;
				characters[go->id] = CharacterRuntime{ ctrl, 0.0f, false };
			}
		}

		accumulator = 0.0f;
		running = true;
	}

	void SyncToScene(Scene& gameScene)
	{
		for (auto& [id, actor] : actors)
		{
			auto* dyn = actor->is<PxRigidDynamic>();
			if (!dyn) continue;
			GameObject* go = gameScene.FindById(id);
			if (!go) continue;

			bool kinematic = dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC;
			if (kinematic)
			{
				// Script/editor drives the transform; feed PhysX a kinematic target.
				PxVec3 p; PxQuat q; glm::vec3 s;
				DecomposeTR(gameScene.WorldMatrix(go), p, q, s);
				dyn->setKinematicTarget(PxTransform(p, q));
				continue;
			}
			// Dynamic: PhysX owns the pose -> write world pose back as a local transform.
			PxTransform t = dyn->getGlobalPose();
			glm::mat4 world = glm::translate(glm::mat4(1.0f), glm::vec3(t.p.x, t.p.y, t.p.z)) *
				glm::mat4_cast(glm::quat(t.q.w, t.q.x, t.q.y, t.q.z));
			glm::mat4 parentWorld(1.0f);
			if (go->parentId != 0)
				if (GameObject* par = gameScene.FindById(go->parentId)) parentWorld = gameScene.WorldMatrix(par);
			glm::mat4 local = glm::inverse(parentWorld) * world;
			go->transform.position = glm::vec3(local[3]);
			glm::vec3 c0(local[0]), c1(local[1]), c2(local[2]);
			glm::mat3 rot(glm::normalize(c0), glm::normalize(c1), glm::normalize(c2));
			glm::vec3 euler = glm::degrees(glm::eulerAngles(glm::normalize(glm::quat_cast(rot))));
			go->transform.rotationEuler = euler;
		}

		// Character controllers own their world position; write it back (minus the local centre).
		for (auto& [id, cr] : characters)
		{
			if (!cr.controller) continue;
			GameObject* go = gameScene.FindById(id);
			if (!go) continue;
			PxExtendedVec3 p = cr.controller->getPosition();
			glm::vec3 wpos = glm::vec3((float)p.x, (float)p.y, (float)p.z) - go->characterController->center;
			glm::mat4 parentWorld(1.0f);
			if (go->parentId != 0)
				if (GameObject* par = gameScene.FindById(go->parentId)) parentWorld = gameScene.WorldMatrix(par);
			glm::vec4 local = glm::inverse(parentWorld) * glm::vec4(wpos, 1.0f);
			go->transform.position = glm::vec3(local);
		}
	}

	void Step(Scene& gameScene, float dt)
	{
		if (!running || !scene) return;
		events.clear();
		accumulator += glm::min(dt, 0.25f); // avoid spiral of death
		int steps = 0;
		while (accumulator >= fixedDt && steps < 8)
		{
			// Push kinematic targets from current transforms before simulating.
			for (auto& [id, actor] : actors)
			{
				auto* dyn = actor->is<PxRigidDynamic>();
				if (dyn && (dyn->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC))
				{
					GameObject* go = gameScene.FindById(id);
					if (go) { PxVec3 p; PxQuat q; glm::vec3 s; DecomposeTR(gameScene.WorldMatrix(go), p, q, s); dyn->setKinematicTarget(PxTransform(p, q)); }
				}
			}
			scene->simulate(fixedDt);
			scene->fetchResults(true);
			accumulator -= fixedDt;
			++steps;
		}
		SyncToScene(gameScene);
	}

	void End()
	{
		for (auto& [id, cr] : characters) if (cr.controller) cr.controller->release();
		characters.clear();
		if (controllerMgr) { controllerMgr->release(); controllerMgr = nullptr; }
		if (controllerMat) { controllerMat->release(); controllerMat = nullptr; }
		if (scene)
		{
			scene->release();
			scene = nullptr;
		}
		actors.clear();
		events.clear();
		running = false;
	}

	bool MoveCharacter(std::uint32_t id, const glm::vec3& disp, float dt)
	{
		auto it = characters.find(id);
		if (it == characters.end() || !it->second.controller) return false;
		PxControllerFilters filters;
		PxControllerCollisionFlags flags = it->second.controller->move(
			PxVec3(disp.x, disp.y, disp.z), 0.001f, dt, filters);
		it->second.grounded = flags.isSet(PxControllerCollisionFlag::eCOLLISION_DOWN);
		return it->second.grounded;
	}

	PxRigidDynamic* Dyn(std::uint32_t id) const
	{
		auto it = actors.find(id);
		return (it != actors.end()) ? it->second->is<PxRigidDynamic>() : nullptr;
	}
};

// ---------------------------------------------------------------------------------------
// Public wrapper
// ---------------------------------------------------------------------------------------
PhysicsScene::PhysicsScene() : impl(std::make_unique<Impl>())
{
	if (!impl->Init())
		std::cerr << "[Physics] PhysX failed to initialize; physics disabled." << std::endl;
}
PhysicsScene::~PhysicsScene() { if (impl) impl->Shutdown(); }

bool PhysicsScene::Available() const { return impl && impl->physics != nullptr; }
void PhysicsScene::SetGravity(const glm::vec3& g) { impl->gravity = g; if (impl->scene) impl->scene->setGravity(PxVec3(g.x, g.y, g.z)); }

void PhysicsScene::Begin(Scene& scene) { impl->Begin(scene); }
void PhysicsScene::Step(Scene& scene, float dt) { impl->Step(scene, dt); }
void PhysicsScene::End() { impl->End(); }
bool PhysicsScene::IsRunning() const { return impl->running; }

void PhysicsScene::SetLinearVelocity(std::uint32_t id, const glm::vec3& v)
{
	if (auto* d = impl->Dyn(id)) d->setLinearVelocity(PxVec3(v.x, v.y, v.z));
}
glm::vec3 PhysicsScene::GetLinearVelocity(std::uint32_t id) const
{
	if (auto* d = impl->Dyn(id)) { PxVec3 v = d->getLinearVelocity(); return glm::vec3(v.x, v.y, v.z); }
	return glm::vec3(0.0f);
}
void PhysicsScene::AddForce(std::uint32_t id, const glm::vec3& f)
{
	if (auto* d = impl->Dyn(id)) d->addForce(PxVec3(f.x, f.y, f.z), PxForceMode::eFORCE);
}
void PhysicsScene::AddImpulse(std::uint32_t id, const glm::vec3& f)
{
	if (auto* d = impl->Dyn(id)) d->addForce(PxVec3(f.x, f.y, f.z), PxForceMode::eIMPULSE);
}

bool PhysicsScene::HasCharacter(std::uint32_t id) const
{
	auto it = impl->characters.find(id);
	return it != impl->characters.end() && it->second.controller != nullptr;
}
bool PhysicsScene::MoveCharacter(std::uint32_t id, const glm::vec3& disp, float dt)
{
	return impl->MoveCharacter(id, disp, dt);
}
bool PhysicsScene::IsCharacterGrounded(std::uint32_t id) const
{
	auto it = impl->characters.find(id);
	return it != impl->characters.end() && it->second.grounded;
}

bool PhysicsScene::Raycast(const glm::vec3& o, const glm::vec3& dir, float maxDist, RaycastHit& out) const
{
	out = RaycastHit{};
	if (!impl->scene) return false;
	glm::vec3 d = glm::length(dir) > 1e-6f ? glm::normalize(dir) : glm::vec3(0, 0, -1);
	PxRaycastBuffer buf;
	if (impl->scene->raycast(PxVec3(o.x, o.y, o.z), PxVec3(d.x, d.y, d.z), maxDist, buf) && buf.hasBlock)
	{
		out.hit = true;
		out.objectId = IdOf(buf.block.actor);
		out.point = glm::vec3(buf.block.position.x, buf.block.position.y, buf.block.position.z);
		out.normal = glm::vec3(buf.block.normal.x, buf.block.normal.y, buf.block.normal.z);
		out.distance = buf.block.distance;
	}
	return out.hit;
}

bool PhysicsScene::SphereCast(const glm::vec3& o, float radius, const glm::vec3& dir, float maxDist, RaycastHit& out) const
{
	out = RaycastHit{};
	if (!impl->scene) return false;
	glm::vec3 d = glm::length(dir) > 1e-6f ? glm::normalize(dir) : glm::vec3(0, 0, -1);
	PxSweepBuffer buf;
	PxTransform pose(PxVec3(o.x, o.y, o.z));
	if (impl->scene->sweep(PxSphereGeometry(radius), pose, PxVec3(d.x, d.y, d.z), maxDist, buf) && buf.hasBlock)
	{
		out.hit = true;
		out.objectId = IdOf(buf.block.actor);
		out.point = glm::vec3(buf.block.position.x, buf.block.position.y, buf.block.position.z);
		out.normal = glm::vec3(buf.block.normal.x, buf.block.normal.y, buf.block.normal.z);
		out.distance = buf.block.distance;
	}
	return out.hit;
}

int PhysicsScene::OverlapSphere(const glm::vec3& c, float radius, std::vector<std::uint32_t>& outIds) const
{
	outIds.clear();
	if (!impl->scene) return 0;
	const PxU32 kMax = 64;
	PxOverlapHit hits[kMax];
	PxOverlapBuffer buf(hits, kMax);
	PxTransform pose(PxVec3(c.x, c.y, c.z));
	if (impl->scene->overlap(PxSphereGeometry(radius), pose, buf))
		for (PxU32 i = 0; i < buf.getNbTouches(); ++i)
			outIds.push_back(IdOf(buf.getTouch(i).actor));
	return (int)outIds.size();
}

const std::vector<ContactEvent>& PhysicsScene::Events() const { return impl->events; }
void PhysicsScene::SetFixedTimestep(float dt) { if (dt > 0.0f) impl->fixedDt = dt; }
float PhysicsScene::FixedTimestep() const { return impl->fixedDt; }

} // namespace ogl