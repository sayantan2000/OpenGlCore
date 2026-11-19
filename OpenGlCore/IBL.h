#ifndef IBL_H
#define IBL_H

#include <glad/glad.h>
#include <glm/glm.hpp>

// Image-Based Lighting: builds the diffuse irradiance cubemap, the pre-filtered specular
// environment cubemap, and the BRDF integration LUT from a procedural HDR sky (no
// external .hdr asset). Owns all GL resources and frees them on destruction.
class IBL
{
public:
	IBL();
	~IBL();
	IBL(const IBL&) = delete;
	IBL& operator=(const IBL&) = delete;

	GLuint IrradianceMap() const { return irradianceMap; }
	GLuint PrefilterMap() const { return prefilterMap; }
	GLuint BrdfLUT() const { return brdfLUT; }
	float  MaxReflectionLod() const { return (float)(prefilterMipLevels - 1); }

	// Draws the environment as a skybox (call after the scene; outputs linear HDR).
	void RenderBackground(const glm::mat4& projection, const glm::mat4& view);

private:
	GLuint envCubemap = 0;
	GLuint irradianceMap = 0;
	GLuint prefilterMap = 0;
	GLuint brdfLUT = 0;
	GLuint cubeVAO = 0, cubeVBO = 0;
	int prefilterMipLevels = 5;

	class Shader* backgroundShader = nullptr;

	void RenderCube();
};

#endif
