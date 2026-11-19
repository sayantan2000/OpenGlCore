#ifndef PBR_MATERIAL_H
#define PBR_MATERIAL_H

#include <glm/glm.hpp>
#include <glad/glad.h>
#include "shaderClass.h"

// A metallic-roughness PBR material. Scalar factors are used directly; optional texture
// maps override them when present. Texture units 3..7 are reserved for material maps
// (0..2 are used by the IBL textures - see Renderer/pbr.frag).
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

	// Sets material uniforms and binds any present maps. Shader must be active.
	void Bind(Shader& shader) const;
};

#endif
