#include "IBL.h"
#include "shaderClass.h"
#include "GLUtils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <stb/stb.h>
#include <tinyexr.h>

// Loads an equirectangular .hdr into a float RGB texture (no clamping). Returns 0 on failure.
static GLuint LoadEquirectHDR(const std::string& path, int& outW, int& outH)
{
	stbi_set_flip_vertically_on_load(true);
	int w = 0, h = 0, n = 0;
	float* data = stbi_loadf(path.c_str(), &w, &h, &n, 0);
	stbi_set_flip_vertically_on_load(false);
	if (!data)
		return 0;

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	// RGB16F preserves HDR range; source channels may be 3 or 4.
	GLenum srcFormat = (n == 4) ? GL_RGBA : GL_RGB;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, srcFormat, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);
	outW = w; outH = h;
	return tex;
}

// Loads an OpenEXR equirectangular image (via tinyexr + zlib) into a float RGB16F texture.
// Flipped vertically to match the .hdr orientation. Returns 0 on failure.
static GLuint LoadEquirectEXR(const std::string& path, int& outW, int& outH)
{
	float* rgba = nullptr;
	int w = 0, h = 0;
	const char* err = nullptr;
	int ret = LoadEXR(&rgba, &w, &h, path.c_str(), &err);
	if (ret != TINYEXR_SUCCESS)
	{
		if (err) { std::cerr << "[IBL] EXR load error: " << err << std::endl; FreeEXRErrorMessage(err); }
		return 0;
	}

	std::vector<float> flipped((size_t)w * h * 4);
	for (int y = 0; y < h; ++y)
		std::memcpy(&flipped[(size_t)(h - 1 - y) * w * 4], &rgba[(size_t)y * w * 4], (size_t)w * 4 * sizeof(float));
	free(rgba);

	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGBA, GL_FLOAT, flipped.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	outW = w; outH = h;
	return tex;
}

// Case-insensitive check that `path` ends with `.exr`.
static bool HasExrExtension(const std::string& path)
{
	if (path.size() < 4) return false;
	std::string ext = path.substr(path.size() - 4);
	for (char& c : ext) c = (char)tolower((unsigned char)c);
	return ext == ".exr";
}

// 36-vertex unit cube (position only).
static const float kCubeVerts[] = {
	-1,-1,-1,  1, 1,-1,  1,-1,-1,  1, 1,-1, -1,-1,-1, -1, 1,-1,
	-1,-1, 1,  1,-1, 1,  1, 1, 1,  1, 1, 1, -1, 1, 1, -1,-1, 1,
	-1, 1, 1, -1, 1,-1, -1,-1,-1, -1,-1,-1, -1,-1, 1, -1, 1, 1,
	 1, 1, 1,  1,-1,-1,  1, 1,-1,  1,-1,-1,  1, 1, 1,  1,-1, 1,
	-1,-1,-1,  1,-1,-1,  1,-1, 1,  1,-1, 1, -1,-1, 1, -1,-1,-1,
	-1, 1,-1,  1, 1, 1,  1, 1,-1,  1, 1, 1, -1, 1,-1, -1, 1, 1
};

void IBL::RenderCube()
{
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

IBL::IBL(const std::string& hdriPath)
{
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	// Cube VAO
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);

	// Save current viewport so we can restore it after the capture passes.
	GLint prevViewport[4];
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	// Capture framebuffer
	GLuint captureFBO = 0, captureRBO = 0;
	glGenFramebuffers(1, &captureFBO);
	glGenRenderbuffers(1, &captureRBO);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
		glm::lookAt(glm::vec3(0.0f), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0))
	};

	// 1) Environment cubemap from the procedural sky ----------------------------------
	const int ENV_SIZE = 512;
	glGenTextures(1, &envCubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	for (int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, ENV_SIZE, ENV_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Try the HDRI first; fall back to the procedural sky if it is missing/unreadable.
	GLuint equirectTex = 0;
	int hdrW = 0, hdrH = 0;
	if (!hdriPath.empty())
		equirectTex = HasExrExtension(hdriPath)
			? LoadEquirectEXR(hdriPath, hdrW, hdrH)
			: LoadEquirectHDR(hdriPath, hdrW, hdrH);
	usingHdri = (equirectTex != 0);

	if (usingHdri)
		std::cout << "[IBL] Loaded HDRI environment: " << hdriPath
		          << " (" << hdrW << "x" << hdrH << ")" << std::endl;
	else
		std::cout << "[IBL] No HDRI at '" << hdriPath
		          << "' - using procedural sky. Place an equirectangular .hdr there to use it."
		          << std::endl;

	{
		// HDRI path uses equirect.frag + the equirectangular texture; otherwise sky.frag.
		Shader envShader("./Shader/cubemap.vert",
			usingHdri ? "./Shader/equirect.frag" : "./Shader/sky.frag");
		envShader.Activate();
		envShader.SetUniformMat4("projection", captureProjection);
		if (usingHdri)
		{
			glUniform1i(glGetUniformLocation(envShader.ID, "equirectangularMap"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, equirectTex);
		}
		glViewport(0, 0, ENV_SIZE, ENV_SIZE);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (int i = 0; i < 6; ++i)
		{
			glm::mat4 v = captureViews[i];
			envShader.SetUniformMat4("view", v);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			RenderCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	if (equirectTex != 0)
		glDeleteTextures(1, &equirectTex); // only needed for the capture
	// Mipmaps for the env map (prefilter samples them to reduce fireflies).
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// 2) Irradiance cubemap -----------------------------------------------------------
	const int IRR_SIZE = 32;
	glGenTextures(1, &irradianceMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
	for (int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, IRR_SIZE, IRR_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRR_SIZE, IRR_SIZE);
	{
		Shader irrShader("./Shader/cubemap.vert", "./Shader/irradiance.frag");
		irrShader.Activate();
		irrShader.SetUniformMat4("projection", captureProjection);
		glUniform1i(glGetUniformLocation(irrShader.ID, "environmentMap"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glViewport(0, 0, IRR_SIZE, IRR_SIZE);
		for (int i = 0; i < 6; ++i)
		{
			glm::mat4 v = captureViews[i];
			irrShader.SetUniformMat4("view", v);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			RenderCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// 3) Pre-filtered specular cubemap (with mips) ------------------------------------
	const int PRE_SIZE = 128;
	glGenTextures(1, &prefilterMap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	for (int i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, PRE_SIZE, PRE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	{
		Shader preShader("./Shader/cubemap.vert", "./Shader/prefilter.frag");
		preShader.Activate();
		preShader.SetUniformMat4("projection", captureProjection);
		glUniform1i(glGetUniformLocation(preShader.ID, "environmentMap"), 0);
		glUniform1f(glGetUniformLocation(preShader.ID, "resolution"), (float)ENV_SIZE);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (int mip = 0; mip < prefilterMipLevels; ++mip)
		{
			int mipW = (int)(PRE_SIZE * std::pow(0.5, mip));
			int mipH = mipW;
			glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipW, mipH);
			glViewport(0, 0, mipW, mipH);

			float roughness = (float)mip / (float)(prefilterMipLevels - 1);
			glUniform1f(glGetUniformLocation(preShader.ID, "roughness"), roughness);
			for (int i = 0; i < 6; ++i)
			{
				glm::mat4 v = captureViews[i];
				preShader.SetUniformMat4("view", v);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				RenderCube();
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	// 4) BRDF integration LUT ---------------------------------------------------------
	const int LUT_SIZE = 512;
	glGenTextures(1, &brdfLUT);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, LUT_SIZE, LUT_SIZE, 0, GL_RG, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, LUT_SIZE, LUT_SIZE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT, 0);
	glViewport(0, 0, LUT_SIZE, LUT_SIZE);
	{
		Shader brdfShader("./Shader/hdr.vert", "./Shader/brdf.frag");
		brdfShader.Activate();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLuint emptyVAO = 0;
		glGenVertexArrays(1, &emptyVAO);
		glBindVertexArray(emptyVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3); // fullscreen triangle
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &emptyVAO);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteFramebuffers(1, &captureFBO);
	glDeleteRenderbuffers(1, &captureRBO);

	// Restore the window viewport.
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

	backgroundShader = new Shader("./Shader/background.vert", "./Shader/background.frag");
	backgroundShader->Activate();
	glUniform1i(glGetUniformLocation(backgroundShader->ID, "environmentMap"), 0);

	glCheckError();
}

void IBL::RenderBackground(const glm::mat4& projection, const glm::mat4& view, float intensity)
{
	backgroundShader->Activate();
	backgroundShader->SetUniformMat4("projection", const_cast<glm::mat4&>(projection));
	backgroundShader->SetUniformMat4("view", const_cast<glm::mat4&>(view));
	glUniform1f(glGetUniformLocation(backgroundShader->ID, "u_bgIntensity"), intensity);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glDepthFunc(GL_LEQUAL); // depth == 1.0 from the shader; pass when <= buffer
	RenderCube();
	glDepthFunc(GL_LESS);
}

IBL::~IBL()
{
	if (envCubemap) glDeleteTextures(1, &envCubemap);
	if (irradianceMap) glDeleteTextures(1, &irradianceMap);
	if (prefilterMap) glDeleteTextures(1, &prefilterMap);
	if (brdfLUT) glDeleteTextures(1, &brdfLUT);
	if (cubeVBO) glDeleteBuffers(1, &cubeVBO);
	if (cubeVAO) glDeleteVertexArrays(1, &cubeVAO);
	delete backgroundShader;
}
