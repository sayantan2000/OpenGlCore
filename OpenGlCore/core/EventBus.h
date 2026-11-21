#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <cstdint>
#include <functional>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace ogl {

// Opaque handle returned by Subscribe; pass it to Unsubscribe to detach a listener.
using SubscriptionId = std::uint64_t;

// A centralized, type-safe publish/subscribe event bus. Events are plain structs identified by
// their C++ type (std::type_index), so there is no string matching or RTTI casting at the call
// site. Listeners can subscribe/unsubscribe at any time, including from inside a dispatch (the
// current dispatch works on a snapshot and removals are deferred, so there are no dangling
// callbacks or iterator invalidation). One EventBus is owned by the Application and shared with
// the scripting, physics-dispatch and editor layers.
//
// Usage:
//   struct DamageEvent { std::uint32_t target; float amount; };
//   auto id = bus.Subscribe<DamageEvent>([](const DamageEvent& e){ ... });
//   bus.Publish(DamageEvent{ id, 10.0f });
//   bus.Unsubscribe(id);
class EventBus
{
public:
	// Registers `fn` to be called for every Publish<T>. Returns a handle for Unsubscribe.
	template <typename T>
	SubscriptionId Subscribe(std::function<void(const T&)> fn)
	{
		const SubscriptionId id = ++nextId;
		auto& list = listeners[std::type_index(typeid(T))];
		list.push_back(Listener{ id, [fn = std::move(fn)](const void* e) { fn(*static_cast<const T*>(e)); } });
		return id;
	}

	// Dispatches `evt` to every current listener for type T. Safe to call re-entrantly and safe
	// for a listener to Subscribe/Unsubscribe during the call.
	template <typename T>
	void Publish(const T& evt)
	{
		auto it = listeners.find(std::type_index(typeid(T)));
		if (it == listeners.end()) return;
		++dispatchDepth;
		// Iterate by index: new subscribers appended during dispatch are intentionally skipped
		// (they take effect on the next Publish), and removals are deferred until unwind.
		const auto& list = it->second;
		for (size_t i = 0, n = list.size(); i < n; ++i)
		{
			const Listener& l = it->second[i];
			if (pendingRemovals.find(l.id) == pendingRemovals.end())
				l.fn(&evt);
		}
		if (--dispatchDepth == 0) FlushRemovals();
	}

	// Detaches a listener by handle. If called during a dispatch the removal is deferred until
	// the outermost dispatch unwinds (so the in-flight listener list stays valid).
	void Unsubscribe(SubscriptionId id)
	{
		if (dispatchDepth > 0) { pendingRemovals.insert(id); return; }
		RemoveNow(id);
	}

	// Removes all listeners (e.g. on scene teardown / editor shutdown).
	void Clear()
	{
		listeners.clear();
		pendingRemovals.clear();
	}

private:
	struct Listener
	{
		SubscriptionId id;
		std::function<void(const void*)> fn;
	};

	void RemoveNow(SubscriptionId id)
	{
		for (auto& [type, list] : listeners)
		{
			for (size_t i = 0; i < list.size(); ++i)
				if (list[i].id == id) { list.erase(list.begin() + i); return; }
		}
	}

	void FlushRemovals()
	{
		if (pendingRemovals.empty()) return;
		for (SubscriptionId id : pendingRemovals) RemoveNow(id);
		pendingRemovals.clear();
	}

	std::unordered_map<std::type_index, std::vector<Listener>> listeners;
	std::unordered_set<SubscriptionId> pendingRemovals;
	SubscriptionId nextId = 0;
	int dispatchDepth = 0;
};

// RAII wrapper that unsubscribes automatically when it goes out of scope. Use for listeners
// whose lifetime is tied to an owning object, to guarantee no dangling callback after it dies.
class ScopedSubscription
{
public:
	ScopedSubscription() = default;
	ScopedSubscription(EventBus* bus, SubscriptionId id) : bus(bus), id(id) {}
	ScopedSubscription(const ScopedSubscription&) = delete;
	ScopedSubscription& operator=(const ScopedSubscription&) = delete;
	ScopedSubscription(ScopedSubscription&& o) noexcept : bus(o.bus), id(o.id) { o.bus = nullptr; }
	ScopedSubscription& operator=(ScopedSubscription&& o) noexcept
	{
		if (this != &o) { Release(); bus = o.bus; id = o.id; o.bus = nullptr; }
		return *this;
	}
	~ScopedSubscription() { Release(); }
	void Release() { if (bus) { bus->Unsubscribe(id); bus = nullptr; } }

private:
	EventBus* bus = nullptr;
	SubscriptionId id = 0;
};

// ---------------------------------------------------------------------------------------------
// Built-in engine event types. Gameplay code may define its own structs and publish them too.
// ---------------------------------------------------------------------------------------------

// Published when two colliders begin/end touching (mirrors ContactEvent, decoupled from PhysX).
struct CollisionBeginEvent { std::uint32_t a = 0, b = 0; };
struct CollisionEndEvent   { std::uint32_t a = 0, b = 0; };
// Published when an object enters/exits a trigger volume (`trigger` is the trigger's object id).
struct TriggerBeginEvent   { std::uint32_t trigger = 0, other = 0; };
struct TriggerEndEvent     { std::uint32_t trigger = 0, other = 0; };

// Play-mode lifecycle, for systems that need to react to Start/Stop.
struct PlayModeChangedEvent { bool playing = false; };

// A generic named message for lightweight script-to-engine / script-to-script communication
// without defining a dedicated struct. `sender` is the originating object id (0 = engine).
struct GameMessage
{
	std::string name;
	std::uint32_t sender = 0;
	float value = 0.0f;
};


} // namespace ogl
#endif
