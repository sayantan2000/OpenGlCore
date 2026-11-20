#ifndef MODEL_CLASS_H
#define MODEL_CLASS_H

#include<string>
#include<vector>
#include<glm/glm.hpp>
#include"Mesh.h"
#include"shaderClass.h"

// Forward-declared Assimp types (avoid pulling assimp headers into this header).
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiTexture;

// Loads a 3D model via Assimp (FBX, OBJ, glTF, ...) and stores it as a set of Meshes
// in this project's standard 11-float vertex layout (pos3, color3, uv2, normal3), so a
// loaded model can be drawn with the existing default.vert/default.frag shaders.
class Model
{
public:
	explicit Model(const std::string& path);
	~Model();
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	// Draws every sub-mesh. For each: if it has an imported diffuse texture, binds it and
	// sets hasTexture=1; otherwise sets the material diffuse color via the "albedo" uniform
	// and hasTexture=0. Caller activates `shader` and sets the shared uniforms first.
	void Draw(Shader& shader);

	// Draws with the PBR shader: sets u_albedo (+ u_albedoMap on unit 3 when textured),
	// and non-metallic defaults. Other PBR maps are disabled (basic imports have none).
	void DrawPBR(Shader& shader);

	// Draws only the geometry (no material/uniform/texture changes). Use when an external
	// PbrMaterial has already been bound to the active shader.
	void DrawGeometry();

	size_t MeshCount() const { return meshes.size(); }

	// Axis-aligned bounds of the loaded geometry (model space).
	glm::vec3 BoundsMin() const { return boundsMin; }
	glm::vec3 BoundsMax() const { return boundsMax; }
	glm::vec3 Size() const { return boundsMax - boundsMin; }
	glm::vec3 Center() const { return (boundsMin + boundsMax) * 0.5f; }
	// Uniform scale factor that makes the largest dimension equal `targetSize` units.
	float ScaleToFit(float targetSize) const;

private:
	struct SubMesh
	{
		Mesh mesh;
		glm::vec3 albedo;       // imported material diffuse color (default grey)
		GLuint textureID = 0;   // imported diffuse texture, 0 = none (use albedo)
	};
	std::vector<SubMesh> meshes;
	std::string directory;      // folder of the model file (for external textures)
	glm::vec3 boundsMin = glm::vec3(0.0f);
	glm::vec3 boundsMax = glm::vec3(0.0f);

	// Recursively walks the node hierarchy, accumulating each node's transform and
	// baking it into the vertices of the meshes that node references.
	void ProcessNode(const aiNode* node, const aiScene* scene, const glm::mat4& parentTransform);
	void AddMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& transform);
	// Texture helpers (return a GL texture id, or 0 on failure).
	GLuint LoadTextureFromFile(const std::string& path);
	GLuint LoadEmbeddedTexture(const aiTexture* tex);
};
#endif
