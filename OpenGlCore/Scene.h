#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include <cstdint>
#include "GameObject.h"

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

	// Removes by id; returns the detached object (for undo) or nullptr.
	std::unique_ptr<GameObject> RemoveById(std::uint32_t id)
	{
		for (size_t i = 0; i < objects.size(); ++i)
		{
			if (objects[i]->id == id)
			{
				auto detached = std::move(objects[i]);
				objects.erase(objects.begin() + i);
				if (selectedId == id) selectedId = 0;
				return detached;
			}
		}
		return nullptr;
	}

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

#endif
