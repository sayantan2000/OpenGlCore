#ifndef PBR_MATERIAL_H
#define PBR_MATERIAL_H

#include <string>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "shaderClass.h"

namespace ogl {

// A metallic-roughness PBR material. Scalar factors are used directly; optional texture
// maps override them when present. Texture units 3..7 are reserved for material maps
// (0..2 are used by the IBL textures - see Renderer/pbr.frag).
//
// The GLuint handles are the live GL textures used for rendering; the matching *Path strings
// record where each map came from so the material can be serialized and re-resolved through
// the AssetManager on load (empty path = no map).
struct PbrMaterial
{
	glm::vec3 albedo = glm::vec3(1.0f);
	float metallic = 0.0f;
	float roughness = 0.5f;
	float ao = 1.0f;

	GLuint albedoMap = 0;
	GLuint normalMap = 0;
	GLuint metallicMap = 0;
	GLuint roughnessMap = 0;
	GLuint aoMap = 0;

	std::string albedoMapPath;
	std::string normalMapPath;
	std::string metallicMapPath;
	std::string roughnessMapPath;
	std::string aoMapPath;

	// Sets material uniforms and binds any present maps. Shader must be active.
	void Bind(Shader& shader) const;
};


} // namespace ogl
#endif
