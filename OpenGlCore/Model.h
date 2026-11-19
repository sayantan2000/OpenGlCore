#ifndef MODEL_CLASS_H
#define MODEL_CLASS_H

#include<string>
#include<vector>
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

private:
	std::vector<Mesh> meshes;
};
#endif
