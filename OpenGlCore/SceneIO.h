#ifndef SCENE_IO_H
#define SCENE_IO_H

#include <string>

class Scene;
class AssetManager;
struct PrimitiveLibrary;

// JSON scene serialization. Save writes the whole Scene (objects, transforms, parent links,
// material scalars + texture paths, lights). Load clears the Scene and rebuilds it, resolving
// geometry through the PrimitiveLibrary (primitives) / AssetManager (models) and material maps
// through the AssetManager, so object ids and references round-trip. Returns false on error.
namespace SceneIO
{
	bool Save(Scene& scene, const std::string& path, std::string* error = nullptr);
	bool Load(Scene& scene, const std::string& path,
		AssetManager& assets, const PrimitiveLibrary& primitives, std::string* error = nullptr);
}

#endif
