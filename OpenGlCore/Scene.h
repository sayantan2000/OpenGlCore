#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <memory>
#include "GameObject.h"

// Owns the scene's GameObjects and the current selection. Lightweight: a flat list plus a
// selected index (the Inspector reads/writes the selected object's components directly).
class Scene
{
public:
	GameObject* Add(std::unique_ptr<GameObject> go)
	{
		objects.push_back(std::move(go));
		return objects.back().get();
	}

	void Remove(size_t index)
	{
		if (index >= objects.size()) return;
		objects.erase(objects.begin() + index);
		if (objects.empty()) selected = -1;
		else if (selected >= (int)objects.size()) selected = (int)objects.size() - 1;
	}

	size_t Count() const { return objects.size(); }
	GameObject* At(size_t i) { return i < objects.size() ? objects[i].get() : nullptr; }

	int Selected() const { return selected; }
	void Select(int i) { selected = (i >= 0 && i < (int)objects.size()) ? i : -1; }
	GameObject* SelectedObject() { return (selected >= 0 && selected < (int)objects.size()) ? objects[selected].get() : nullptr; }

	std::vector<std::unique_ptr<GameObject>>& Objects() { return objects; }

private:
	std::vector<std::unique_ptr<GameObject>> objects;
	int selected = -1;
};

#endif
