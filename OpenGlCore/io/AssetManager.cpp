#include "AssetManager.h"
#include "Texture.h"
#include "Model.h"
#include <iostream>

namespace ogl {

AssetManager::AssetManager() = default;
AssetManager::~AssetManager() = default; // unique_ptrs free the GL resources (RAII)

GLuint AssetManager::GetTexture(const std::string& path, bool /*srgb*/)
{
	if (path.empty()) return 0;

	auto it = textures.find(path);
	if (it != textures.end())
		return it->second ? it->second->ID : 0;

	try
	{
		auto tex = std::make_unique<Texture>(path.c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
		GLuint id = tex->ID;
		textures.emplace(path, std::move(tex));
		return id;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[AssetManager] Failed to load texture '" << path << "': " << e.what() << std::endl;
		textures.emplace(path, nullptr); // negative-cache so we don't retry every frame
		return 0;
	}
}

Model* AssetManager::GetModel(const std::string& path)
{
	if (path.empty()) return nullptr;

	auto it = models.find(path);
	if (it != models.end())
		return it->second.get();

	try
	{
		auto model = std::make_unique<Model>(path);
		Model* raw = model.get();
		models.emplace(path, std::move(model));
		return raw;
	}
	catch (const std::exception& e)
	{
		std::cerr << "[AssetManager] Failed to load model '" << path << "': " << e.what() << std::endl;
		models.emplace(path, nullptr);
		return nullptr;
	}
}

} // namespace ogl