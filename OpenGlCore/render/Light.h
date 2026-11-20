#ifndef LIGHT_H
#define LIGHT_H

#include <vector>
#include <glm/glm.hpp>
#include "shaderClass.h"

namespace ogl {

// Light definitions for the PBR renderer. Colors are linear radiance; `intensity`
// scales the color. Point/spot lights use physically-based inverse-square falloff
// in the shader (no manual constant/linear/quadratic terms).

struct DirectionalLight
{
	glm::vec3 direction = glm::vec3(-0.3f, -1.0f, -0.4f); // world-space, points FROM the light
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 2.0f;
};

struct PointLight
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 20.0f;
};

struct SpotLight
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 30.0f;
	float innerCutoffDeg = 12.5f; // full intensity inside this cone half-angle
	float outerCutoffDeg = 20.0f; // fades to zero by this half-angle
};

// Rectangle (area) light: a finite emitter plane. `normal` is the emission direction,
// `right`/`up` span the plane; `halfWidth`/`halfHeight` are half-extents in world units.
// Shaded with a representative-point approximation (see pbr.frag), not full LTC.
struct RectLight
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
	glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 color = glm::vec3(1.0f);
	float intensity = 5.0f;
	float halfWidth = 1.0f;
	float halfHeight = 0.5f;
};

// Max lights must match the array sizes declared in pbr.frag.
constexpr int MAX_POINT_LIGHTS = 8;
constexpr int MAX_SPOT_LIGHTS = 4;
constexpr int MAX_RECT_LIGHTS = 4;

struct LightSet
{
	bool hasDirectional = true;
	DirectionalLight directional;
	std::vector<PointLight> points;
	std::vector<SpotLight> spots;
	std::vector<RectLight> rects;

	// Uploads all lights to the given (already-activated) PBR shader.
	void Upload(Shader& shader) const;
};


} // namespace ogl
#endif
