#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <string>
#include <memory>
#include <unordered_map>
#include <glad/glad.h>

namespace ogl {

class Texture;
class Model;

// Caches textures and models by file path so each asset is loaded/uploaded to the GPU once
// and shared by every material/renderer that references it. This is what lets materials and
// meshes be stored by path (and therefore serialized) instead of by raw GL handle.
//
// The manager owns the loaded resources for the lifetime of the Application; handles it hands
// out stay valid until the manager is destroyed.
class AssetManager
{
public:
	AssetManager();
	~AssetManager();
	AssetManager(const AssetManager&) = delete;
	AssetManager& operator=(const AssetManager&) = delete;

	// Returns a shared GL texture id for `path` (loads+caches on first request). Returns 0 on
	// empty path or load failure. `srgb` is accepted for intent/documentation; decode-to-linear
	// currently happens in the shader, so it does not change the upload.
	GLuint GetTexture(const std::string& path, bool srgb = false);

	// Returns a shared Model* for `path` (loads+caches on first request), or nullptr on failure.
	Model* GetModel(const std::string& path);

	// True if the path is already cached.
	bool HasTexture(const std::string& path) const { return textures.count(path) != 0; }
	bool HasModel(const std::string& path) const { return models.count(path) != 0; }

private:
	std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
	std::unordered_map<std::string, std::unique_ptr<Model>> models;
};


} // namespace ogl
#endif
