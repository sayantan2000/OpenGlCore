#include "Light.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <algorithm>
#include <cmath>

static void SetVec3(GLuint id, const char* name, const glm::vec3& v)
{
	glUniform3fv(glGetUniformLocation(id, name), 1, glm::value_ptr(v));
}
static void SetFloat(GLuint id, const char* name, float v)
{
	glUniform1f(glGetUniformLocation(id, name), v);
}
static void SetInt(GLuint id, const char* name, int v)
{
	glUniform1i(glGetUniformLocation(id, name), v);
}

void LightSet::Upload(Shader& shader) const
{
	const GLuint id = shader.ID;

	// Directional
	SetInt(id, "u_hasDirLight", hasDirectional ? 1 : 0);
	if (hasDirectional)
	{
		SetVec3(id, "u_dirLight.direction", glm::normalize(directional.direction));
		SetVec3(id, "u_dirLight.color", directional.color * directional.intensity);
	}

	// Point lights
	int pc = std::min((int)points.size(), MAX_POINT_LIGHTS);
	SetInt(id, "u_numPointLights", pc);
	for (int i = 0; i < pc; ++i)
	{
		std::string base = "u_pointLights[" + std::to_string(i) + "].";
		SetVec3(id, (base + "position").c_str(), points[i].position);
		SetVec3(id, (base + "color").c_str(), points[i].color * points[i].intensity);
	}

	// Spot lights
	int sc = std::min((int)spots.size(), MAX_SPOT_LIGHTS);
	SetInt(id, "u_numSpotLights", sc);
	for (int i = 0; i < sc; ++i)
	{
		std::string base = "u_spotLights[" + std::to_string(i) + "].";
		SetVec3(id, (base + "position").c_str(), spots[i].position);
		SetVec3(id, (base + "direction").c_str(), glm::normalize(spots[i].direction));
		SetVec3(id, (base + "color").c_str(), spots[i].color * spots[i].intensity);
		// Store cosines of the cone half-angles for cheap comparison in the shader.
		SetFloat(id, (base + "innerCos").c_str(), std::cos(glm::radians(spots[i].innerCutoffDeg)));
		SetFloat(id, (base + "outerCos").c_str(), std::cos(glm::radians(spots[i].outerCutoffDeg)));
	}

	// Rectangle / area lights
	int rc = std::min((int)rects.size(), MAX_RECT_LIGHTS);
	SetInt(id, "u_numRectLights", rc);
	for (int i = 0; i < rc; ++i)
	{
		std::string base = "u_rectLights[" + std::to_string(i) + "].";
		SetVec3(id, (base + "position").c_str(), rects[i].position);
		SetVec3(id, (base + "normal").c_str(), glm::normalize(rects[i].normal));
		SetVec3(id, (base + "right").c_str(), glm::normalize(rects[i].right));
		SetVec3(id, (base + "up").c_str(), glm::normalize(rects[i].up));
		SetVec3(id, (base + "color").c_str(), rects[i].color * rects[i].intensity);
		SetFloat(id, (base + "halfWidth").c_str(), rects[i].halfWidth);
		SetFloat(id, (base + "halfHeight").c_str(), rects[i].halfHeight);
	}
}
