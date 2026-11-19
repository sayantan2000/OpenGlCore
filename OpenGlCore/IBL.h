#ifndef IBL_H
#define IBL_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

// Image-Based Lighting: builds the diffuse irradiance cubemap, the pre-filtered specular
// environment cubemap, and the BRDF integration LUT from an HDR environment. The source is
// an equirectangular .hdr image when `hdriPath` loads successfully; otherwise it falls back
// to a procedural sky (and logs where to place the HDRI). Owns all GL resources (RAII).
class IBL
{
public:
	// hdriPath: equirectangular .hdr to load; empty/missing => procedural sky fallback.
	explicit IBL(const std::string& hdriPath = "");
	~IBL();
	IBL(const IBL&) = delete;
	IBL& operator=(const IBL&) = delete;

	GLuint IrradianceMap() const { return irradianceMap; }
	GLuint PrefilterMap() const { return prefilterMap; }
	GLuint BrdfLUT() const { return brdfLUT; }
	float  MaxReflectionLod() const { return (float)(prefilterMipLevels - 1); }
	bool   UsingHDRI() const { return usingHdri; }

	// Draws the environment as a skybox (call after the scene; outputs linear HDR).
	// `intensity` scales skybox brightness independently of scene exposure.
	void RenderBackground(const glm::mat4& projection, const glm::mat4& view, float intensity = 1.0f);

private:
	bool usingHdri = false;
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
