#include "Physics.h"
#include "Scene.h"
#include "GameObject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace ogl {

// Collider debug wireframes built purely from component data + the object's world transform,
// so visualization works in both edit and play mode without touching PhysX.
namespace
{
	const glm::vec3 kColliderColor(0.2f, 0.9f, 0.4f);
	const glm::vec3 kTriggerColor(0.2f, 0.7f, 1.0f);

	void Line(std::vector<DebugLine>& out, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
	{
		out.push_back({ a, b, c });
	}

	glm::vec3 X(const glm::mat4& m, const glm::vec3& p) { return glm::vec3(m * glm::vec4(p, 1.0f)); }

	void Box(std::vector<DebugLine>& out, const glm::mat4& m, const glm::vec3& c, const glm::vec3& h, const glm::vec3& col)
	{
		glm::vec3 v[8] = {
			c + glm::vec3(-h.x,-h.y,-h.z), c + glm::vec3(h.x,-h.y,-h.z), c + glm::vec3(h.x,h.y,-h.z), c + glm::vec3(-h.x,h.y,-h.z),
			c + glm::vec3(-h.x,-h.y, h.z), c + glm::vec3(h.x,-h.y, h.z), c + glm::vec3(h.x,h.y, h.z), c + glm::vec3(-h.x,h.y, h.z),
		};
		for (auto& p : v) p = X(m, p);
		int e[12][2] = { {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7} };
		for (auto& ed : e) Line(out, v[ed[0]], v[ed[1]], col);
	}

	void Circle(std::vector<DebugLine>& out, const glm::mat4& m, const glm::vec3& c, float r,
		int axis, const glm::vec3& col, int seg = 24)
	{
		glm::vec3 prev;
		for (int i = 0; i <= seg; ++i)
		{
			float a = (float)i / seg * 6.2831853f;
			float s = std::sin(a) * r, cc = std::cos(a) * r;
			glm::vec3 p = (axis == 0) ? glm::vec3(0, cc, s) : (axis == 1) ? glm::vec3(cc, 0, s) : glm::vec3(cc, s, 0);
			p = X(m, c + p);
			if (i > 0) Line(out, prev, p, col);
			prev = p;
		}
	}
}

namespace PhysicsDebug
{
	void BuildColliderLines(Scene& scene, std::vector<DebugLine>& out)
	{
		for (auto& up : scene.Objects())
		{
			GameObject* go = up.get();
			if (!go->active || !go->collider || !go->collider->enabled) continue;
			const ColliderComponent& col = *go->collider;
			glm::mat4 m = scene.WorldMatrix(go);
			glm::vec3 c = col.center;
			const glm::vec3& color = col.isTrigger ? kTriggerColor : kColliderColor;

			switch (col.shape)
			{
			case ColliderComponent::Shape::Box:
				Box(out, m, c, col.halfExtents, color);
				break;
			case ColliderComponent::Shape::Sphere:
				Circle(out, m, c, col.radius, 0, color);
				Circle(out, m, c, col.radius, 1, color);
				Circle(out, m, c, col.radius, 2, color);
				break;
			case ColliderComponent::Shape::Capsule:
			{
				float hh = col.halfHeight;
				Circle(out, m, c + glm::vec3(0, hh, 0), col.radius, 1, color);
				Circle(out, m, c - glm::vec3(0, hh, 0), col.radius, 1, color);
				Circle(out, m, c, col.radius, 0, color);
				Circle(out, m, c, col.radius, 2, color);
				break;
			}
			case ColliderComponent::Shape::Plane:
			{
				float s = 5.0f;
				Line(out, X(m, c + glm::vec3(-s, 0, 0)), X(m, c + glm::vec3(s, 0, 0)), color);
				Line(out, X(m, c + glm::vec3(0, 0, -s)), X(m, c + glm::vec3(0, 0, s)), color);
				Box(out, m, c, glm::vec3(s, 0.001f, s), color);
				break;
			}
			case ColliderComponent::Shape::ConvexMesh:
			case ColliderComponent::Shape::TriangleMesh:
				// Approximate with the renderer's AABB when available.
				if (go->meshRenderer)
				{
					glm::vec3 mn = go->meshRenderer->localMin, mx = go->meshRenderer->localMax;
					Box(out, m, (mn + mx) * 0.5f, (mx - mn) * 0.5f, color);
				}
				break;
			}
		}
	}
}

} // namespace ogl