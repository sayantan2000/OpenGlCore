# Gameplay Systems

Status: **implemented** (event bus, gamepad input, configurable actions, character controller).
Validated headlessly via `OGL_SCENE_SELFTEST`. Visual/interactive behaviour not verified (no display).

## Event Bus (`core/EventBus.h`)
A header-only, type-safe publish/subscribe bus. Events are plain structs identified by their C++
type (`std::type_index`) — no string matching at the call site.

```cpp
struct DamageEvent { std::uint32_t target; float amount; };
SubscriptionId id = bus.Subscribe<DamageEvent>([](const DamageEvent& e){ /* ... */ });
bus.Publish(DamageEvent{ target, 10.0f });
bus.Unsubscribe(id);                       // or wrap in ScopedSubscription for RAII
```

- **Re-entrant-safe**: a listener may Subscribe/Unsubscribe during a dispatch. The in-flight
  dispatch works on the live list by index; removals are deferred until the outermost dispatch
  unwinds, so there are no dangling callbacks or iterator invalidation.
- **Ownership**: one `EventBus` is owned by `Application` (`events`) and set on the `ScriptSystem`
  (`SetEventBus`). Scripts reach it through `ScriptContext::events`.
- **Built-in events**: `CollisionBeginEvent` / `CollisionEndEvent` / `TriggerBeginEvent` /
  `TriggerEndEvent` (re-published from PhysX contacts by `ScriptSystem::DispatchPhysicsEvents`),
  `PlayModeChangedEvent`, and a generic `GameMessage { name, sender, value }` for lightweight
  script-to-engine / script-to-script messaging.
- **Lifecycle**: `ScopedSubscription` unsubscribes automatically on destruction.

## Gamepad Input & Actions (`scripting/Input.{h,cpp}`)
`Input` now covers keyboard, mouse **and** gamepad in one abstraction, polled once per frame.

- Up to `Input::kMaxPads` (4) controllers via the GLFW gamepad API (`glfwGetGamepadState`).
- Per-pad queries: `GetGamepadButton(Down/Up)`, `GetGamepadAxis`, `LeftStick`/`RightStick`
  (y-up), `LeftTrigger`/`RightTrigger` ([0,1]), `IsGamepadConnected`, `GamepadName`.
- **Deadzone** configurable via `SetDeadzone` (default 0.15); triggers are remapped to [0,1].
- The left stick is folded into the existing `GetAxis("Horizontal"/"Vertical")` named axes, so
  existing keyboard-driven scripts gain stick support for free.
- **Configurable actions**: `BindAction(name, {keys, mouseButtons, padButtons})` then
  `GetAction(name)` / `GetActionDown(name)`. Defaults: `Jump`, `Fire`, `Sprint`, `Cancel`,
  `Submit`. Bindings are data, so a future settings UI or config file can rebind without code.

## Character Controller
See **docs/PHYSICS.md → Character controller**. Summary: `CharacterControllerComponent` +
PhysX `PxCapsuleController` (swept kinematic capsule), moved via
`PhysicsScene::MoveCharacter`/`IsCharacterGrounded`, with a built-in `CharacterController` script
(WASD/stick + gravity + jump + ground detection), Inspector section, serialization and debug viz.

## Validation
`OGL_SCENE_SELFTEST=1 OpenGlCore.exe` prints and checks:
- `EventBus hits=2 lastSender=7` — publish delivers to the subscriber, unsubscribe stops delivery.
- `Character hasCtrl=1 grounded=1 y1=…` — the PxController is created, falls under gravity, and
  settles grounded on the scene's ground collider.

## Known limitations
- Gamepad rebinding has no UI yet (API + defaults only); no per-action analog value accessor.
- Event bus is single-threaded (matches the engine's single-threaded game loop).
- Character controller: no built-in slope-slide, crouch, or moving-platform carry beyond PhysX
  defaults.
