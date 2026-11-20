#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include <cstdint>
#include "GameObject.h"

namespace ogl {

// Owns the scene's GameObjects and the current selection. Lightweight: a flat list plus a
// selection tracked by stable object id (so create/delete/undo don't corrupt it). The
// Inspector reads/writes the selected object's components directly.
class Scene
{
public:
	// Adds an object, assigning it a fresh stable id if it doesn't already have one.
	GameObject* Add(std::unique_ptr<GameObject> go)
	{
		if (go->id == 0) go->id = nextId++;
		else nextId = (go->id >= nextId) ? go->id + 1 : nextId; // keep ids monotonic on restore
		objects.push_back(std::move(go));
		return objects.back().get();
	}

	// Inserts at a specific index (used when undoing a deletion to restore original order).
	GameObject* InsertAt(size_t index, std::unique_ptr<GameObject> go)
	{
		if (go->id == 0) go->id = nextId++;
		else nextId = (go->id >= nextId) ? go->id + 1 : nextId;
		if (index > objects.size()) index = objects.size();
		auto* raw = go.get();
		objects.insert(objects.begin() + index, std::move(go));
		return raw;
	}

	void Remove(size_t index)
	{
		if (index >= objects.size()) return;
		std::uint32_t removedId = objects[index]->id;
		objects.erase(objects.begin() + index);
		if (selectedId == removedId) selectedId = 0;
	}

	// Removes by id; returns the detached object (for undo) or nullptr. Any children are
	// promoted to the removed object's parent so no object is left orphaned.
	std::unique_ptr<GameObject> RemoveById(std::uint32_t id)
	{
		for (size_t i = 0; i < objects.size(); ++i)
		{
			if (objects[i]->id == id)
			{
				std::uint32_t grandparent = objects[i]->parentId;
				auto detached = std::move(objects[i]);
				objects.erase(objects.begin() + i);
				for (auto& o : objects)
					if (o->parentId == id) o->parentId = grandparent;
				if (selectedId == id) selectedId = 0;
				return detached;
			}
		}
		return nullptr;
	}

	void Clear() { objects.clear(); selectedId = 0; nextId = 1; }

	size_t Count() const { return objects.size(); }
	GameObject* At(size_t i) { return i < objects.size() ? objects[i].get() : nullptr; }

	GameObject* FindById(std::uint32_t id)
	{
		for (auto& o : objects) if (o->id == id) return o.get();
		return nullptr;
	}

	int IndexOfId(std::uint32_t id) const
	{
		for (size_t i = 0; i < objects.size(); ++i) if (objects[i]->id == id) return (int)i;
		return -1;
	}

	// Selection is stored by id; Selected()/Select(int) keep the old index-based API working.
	int Selected() const { return IndexOfId(selectedId); }
	void Select(int i) { selectedId = (i >= 0 && i < (int)objects.size()) ? objects[i]->id : 0; }

	std::uint32_t SelectedId() const { return selectedId; }
	void SelectById(std::uint32_t id) { selectedId = FindByIdConst(id) ? id : 0; }
	void Deselect() { selectedId = 0; }

	GameObject* SelectedObject()
	{
		int i = Selected();
		return (i >= 0) ? objects[i].get() : nullptr;
	}

	std::vector<std::unique_ptr<GameObject>>& Objects() { return objects; }

	// --- Hierarchy ------------------------------------------------------------------------
	// World matrix = (parent world) * (local). Walks the parent chain with a depth guard so a
	// corrupt/cyclic link can't loop forever.
	glm::mat4 WorldMatrix(const GameObject* go)
	{
		glm::mat4 m = go ? go->transform.Matrix() : glm::mat4(1.0f);
		std::uint32_t pid = go ? go->parentId : 0;
		for (int guard = 0; pid != 0 && guard < 256; ++guard)
		{
			GameObject* p = FindById(pid);
			if (!p) break;
			m = p->transform.Matrix() * m;
			pid = p->parentId;
		}
		return m;
	}

	glm::vec3 WorldPosition(const GameObject* go) { return glm::vec3(WorldMatrix(go)[3]); }

	// True if `ancestor` is `node` or any of its ancestors (used to prevent reparent cycles).
	bool IsAncestorOf(std::uint32_t ancestor, std::uint32_t node)
	{
		for (int guard = 0; node != 0 && guard < 256; ++guard)
		{
			if (node == ancestor) return true;
			GameObject* n = FindById(node);
			if (!n) break;
			node = n->parentId;
		}
		return false;
	}

	// Reparents `childId` under `parentId` (0 = detach to root). Rejects cycles. Does not
	// change the local transform (so the object may visually move; the editor can compensate).
	bool SetParent(std::uint32_t childId, std::uint32_t parentId)
	{
		if (childId == 0 || childId == parentId) return false;
		if (parentId != 0 && IsAncestorOf(childId, parentId)) return false; // would create a cycle
		GameObject* c = FindById(childId);
		if (!c) return false;
		c->parentId = parentId;
		return true;
	}

	// Reserves and returns the next id without consuming it (for pre-building commands).
	std::uint32_t PeekNextId() const { return nextId; }

private:
	bool FindByIdConst(std::uint32_t id) const
	{
		for (auto& o : objects) if (o->id == id) return true;
		return false;
	}

	std::vector<std::unique_ptr<GameObject>> objects;
	std::uint32_t selectedId = 0;     // 0 = nothing selected
	std::uint32_t nextId = 1;         // ids start at 1 (0 means "unassigned")
};


} // namespace ogl
#endif
