#ifndef MODEL_CLASS_H
#define MODEL_CLASS_H

#include<string>
#include<vector>
#include<glm/glm.hpp>
#include"Mesh.h"

// Loads a 3D model via Assimp (FBX, OBJ, glTF, ...) and stores it as a set of Meshes
// in this project's standard 11-float vertex layout (pos3, color3, uv2, normal3), so a
// loaded model can be drawn with the existing default.vert/default.frag shaders.
class Model
{
public:
	explicit Model(const std::string& path);

	// Draws every sub-mesh. Caller binds the shader/texture and sets uniforms first.
	void Draw();

	size_t MeshCount() const { return meshes.size(); }

	// Axis-aligned bounds of the loaded geometry (model space).
	glm::vec3 BoundsMin() const { return boundsMin; }
	glm::vec3 BoundsMax() const { return boundsMax; }
	glm::vec3 Size() const { return boundsMax - boundsMin; }
	glm::vec3 Center() const { return (boundsMin + boundsMax) * 0.5f; }
	// Uniform scale factor that makes the largest dimension equal `targetSize` units.
	float ScaleToFit(float targetSize) const;

private:
	std::vector<Mesh> meshes;
	glm::vec3 boundsMin = glm::vec3(0.0f);
	glm::vec3 boundsMax = glm::vec3(0.0f);
};
#endif
