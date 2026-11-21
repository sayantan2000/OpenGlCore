#ifndef MATERIAL_ASSET_H
#define MATERIAL_ASSET_H

#include <string>
#include <glm/glm.hpp>
#include "PbrMaterial.h"
#include "Guid.h"

namespace ogl {

class AssetDatabase;
class AssetManager;

// A persistent material asset (the on-disk `.mat` file), as opposed to the runtime `PbrMaterial`
// (which holds live GL handles). A MaterialAsset stores stable data only: a GUID, a name, a shader
// reference, scalar PBR properties, and texture references *by GUID* (never raw GL ids or absolute
// paths), plus UV tiling/offset and a couple of rendering flags. It is reconstructible after an
// editor restart and survives moving/renaming its textures (GUIDs are resolved through the
// AssetDatabase at load). `Resolve` turns it into a renderable PbrMaterial.
struct MaterialAsset
{
	Guid guid;
	std::string name = "New Material";
	std::string shader = "pbr";          // shader id (only "pbr" supported today)

	glm::vec3 albedo = glm::vec3(0.8f);
	float metallic = 0.0f;
	float roughness = 0.5f;
	float ao = 1.0f;

	Guid albedoTex, normalTex, metallicTex, roughnessTex, aoTex; // invalid = no map

	glm::vec2 tiling = glm::vec2(1.0f);  // stored + serialized (shader application is a TODO)
	glm::vec2 offset = glm::vec2(0.0f);
	bool doubleSided = false;

	// Resets every field except the GUID and name to engine defaults.
	void ResetToDefaults();

	// Serialize to / from a `.mat` JSON file (absolute path). Load fills `guid` from the file.
	bool Save(const std::string& absPath) const;
	static bool Load(const std::string& absPath, MaterialAsset& out);

	// Build a renderable PbrMaterial: copies scalars and resolves each texture GUID to a shared GL
	// handle (via the AssetDatabase for guid->path and the AssetManager for the actual upload).
	PbrMaterial Resolve(const AssetDatabase& db, AssetManager& assets) const;

	// Fill this asset's scalar/texture fields FROM an existing runtime PbrMaterial, resolving the
	// material's texture *paths* back to GUIDs via the database (used to "extract" a material).
	void CaptureFrom(const PbrMaterial& mat, const AssetDatabase& db);
};


} // namespace ogl
#endif
