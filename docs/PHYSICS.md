# Physics (NVIDIA PhysX 5.5)

## Dependency & build
- **PhysX 5.5.0** via vcpkg (`"physx"` in `vcpkg.json`). Restore with the usual command:
  `& "C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe" install --triplet x64-windows`
  (populates the gitignored `vcpkg_installed/`; the first build of PhysX is long).
- Headers: `vcpkg_installed/x64-windows/include/physx` (added to the project include path);
  code includes `<PxPhysicsAPI.h>`.
- Linked libs (Debug uses `debug/lib`, Release uses `lib`): `PhysX_64`, `PhysXCommon_64`,
  `PhysXFoundation_64`, `PhysXExtensions_static_64`, `PhysXPvdSDK_static_64`, `PhysXCooking_64`.
- Runtime DLLs (`PhysX_64.dll`, `PhysXCommon_64.dll`, `PhysXFoundation_64.dll`, …) are copied
  next to the exe by the existing `CopyVcpkgDlls` build target. No PhysX binaries are committed.

## Architecture
- **Components (plain data, serializable, in `GameObject`)**: `RigidBodyComponent`
  (Static/Dynamic/Kinematic, mass, gravity, linear/angular damping, initial velocities, per-axis
  position/rotation lock constraints) and `ColliderComponent` (Box/Sphere/Capsule/Plane/
  ConvexMesh/TriangleMesh, center offset, dimensions, `isTrigger`, `PhysicsMaterialDesc`
  friction/restitution). Keeping them as data (not PhysX objects) preserves copyability, undo
  snapshots, and JSON serialization.
- **`PhysicsScene`** (`Physics.h` / `PhysicsScene.cpp`) wraps the PhysX SDK behind a pImpl, so no
  other TU sees PhysX headers. It owns the `PxFoundation`/`PxPhysics`/`PxScene`/dispatcher and a
  map of GameObject id → `PxRigidActor` (actor `userData` stores the id for event/query mapping).

## Component lifecycle & Play-mode integration
- Play-mode owns simulation (edit mode never simulates). On **Play** the `Editor` snapshots the
  scene to JSON, then `PhysicsScene::Begin` builds a PhysX actor per object with a collider
  (adding a rigid body if present; otherwise static). On **Stop** the actors are released and the
  JSON snapshot is restored — the edit-time scene is byte-for-byte recovered.
- **Fixed timestep**: `Step` accumulates frame dt and runs `simulate()/fetchResults()` in fixed
  sub-steps (default 1/60 s, configurable, max 8 sub-steps to avoid a spiral of death). After
  stepping, dynamic actor poses are written back into each `GameObject.transform` (world → local
  relative to the parent, scale preserved). Kinematic actors read the transform instead (script/
  editor-driven) via `setKinematicTarget`. Static actors never move. Rendering always reads the
  transform, so render and physics never fight over it.
- World scale is baked into collider dimensions at `Begin` (PhysX geometry can't be arbitrarily
  scaled): box half-extents × scale, sphere radius × average scale, etc.

## Collision / trigger event flow
- A custom filter shader enables `eNOTIFY_TOUCH_FOUND/LOST` for contacts and
  `eTRIGGER_DEFAULT` for triggers. A `PxSimulationEventCallback` collects pairs into a
  `std::vector<ContactEvent>` (by object id), cleared each `Step`.
- After `Step`, `ScriptSystem::DispatchPhysicsEvents` reads `PhysicsScene::Events()` and calls
  `ScriptBehavior::OnCollision(ctx, otherId)` / `OnTrigger(ctx, otherId)` on both involved
  objects' behaviors.

## Scripting integration
- `ScriptContext` carries a `PhysicsScene*`. Behaviors can `Raycast`, `SphereCast`,
  `OverlapSphere`, `SetLinearVelocity`/`GetLinearVelocity`, `AddForce`, `AddImpulse` by object id.
- Per-frame order in Play: `ScriptSystem::Update` (scripts set velocities / move kinematics) →
  `PhysicsScene::Step` (simulate + write transforms) → `DispatchPhysicsEvents`.

## Scene queries
- `Raycast(origin, dir, maxDist, RaycastHit&)`, `SphereCast(...)`, `OverlapSphere(center, radius,
  ids)`. Hits report the GameObject id, point, normal, distance.

## Debug visualization
- `PhysicsDebug::BuildColliderLines` builds collider/trigger wireframes **from component data**
  (independent of PhysX, so it also works in edit mode): box edges, sphere/capsule rings, a plane
  cross, and an AABB fallback for mesh colliders. Rendered by `Application::RenderPhysicsDebug`
  via the `LineRenderer` (green = collider, blue = trigger). Toggle: **View → Physics Debug**.

## Validation
- Headless self-test (`OGL_SCENE_SELFTEST=1 OpenGlCore.exe`): saves+reloads the scene (incl.
  physics components), ticks a script, **drops a dynamic sphere ~2 s** (falls from y=4, settles at
  y≈-0.45), and **cooks a ConvexMesh collider and drops it** (also settles at y≈-0.45), confirming
  hull cooking from retained geometry. Also reports the chair Model's aggregated collision vertex
  count. Prints `[SELFTEST] PASS`. Visual/interaction behavior is not verified here (no display).

## Mesh collider cooking
- `Mesh` retains a CPU copy of positions + indices (`Mesh::Positions()`/`Indices()`, extracted
  from attribute 0 at construction); `Model` aggregates them across sub-meshes
  (`Model::CollisionPositions()`/`CollisionIndices()`, built lazily, index-offset per sub-mesh).
- `PhysicsScene` cooks on demand via the standalone `PxCreateConvexMesh` / `PxCreateTriangleMesh`
  (insertion-callback API — no intermediate stream), scaled with `PxMeshScale` from the object's
  world scale, and **caches** each cooked mesh by the renderer's `sourceId` so duplicate objects
  share one hull. Cooked meshes are released on `PhysicsScene` shutdown.
- **Triangle mesh on a dynamic (non-kinematic) body** is unsupported by PhysX; such a request is
  automatically cooked as a **convex hull** instead (with a warning), matching Unity's behavior.
  Triangle meshes work on static/kinematic bodies. A cook failure falls back to a box.

## Character controller
- `CharacterControllerComponent` (plain data on `GameObject`) drives a PhysX `PxCapsuleController`
  created by `PhysicsScene::Begin` and owned by a `PxControllerManager`. It is a **swept kinematic
  capsule**, not a force-driven rigid body, so movement is stable (no bouncing/tunnelling) with
  automatic step-climbing and slope limiting.
- API: `PhysicsScene::MoveCharacter(id, disp, dt)` sweeps the capsule and returns grounded state;
  `IsCharacterGrounded(id)` / `HasCharacter(id)` query it. The resolved world position is written
  back to the GameObject transform in `Step`.
- The built-in `CharacterController` script (ScriptRegistry) reads WASD/left-stick + the "Sprint"
  and "Jump" actions, integrates gravity on its own vertical velocity, and calls `MoveCharacter`.
  Tuning (move/run speed, jump height, gravity) lives on the component. Inspector + SceneIO + a
  capsule debug wireframe are wired. An object with a controller should not also have a RigidBody.
- Validated headlessly: a capsule dropped from y=3 falls, becomes grounded and settles.

## Known limitations / next steps
- No joints or continuous collision (CCD) yet; the character controller has no built-in
  slope-slide or moving-platform support beyond PhysX defaults.
- GPU/PVD features unused (`PhysXGpu_64.dll` present but not engaged).
