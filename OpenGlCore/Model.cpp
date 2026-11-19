#include "Model.h"
#include <stdexcept>
#include <iostream>
#include <limits>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Assimp matrices are row-major; glm is column-major, so transpose on conversion.
static glm::mat4 ToGlm(const aiMatrix4x4& m)
{
	return glm::mat4(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4);
}

Model::Model(const std::string& path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path,
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs);

	if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
	{
		std::string msg = std::string("Assimp: failed to load model '") + path +
			"': " + importer.GetErrorString();
		std::cerr << "[MODEL ERROR] " << msg << std::endl;
		throw std::runtime_error(msg);
	}

	boundsMin = glm::vec3(std::numeric_limits<float>::max());
	boundsMax = glm::vec3(std::numeric_limits<float>::lowest());

	// Walk the hierarchy so per-node transforms are applied (baked into vertices).
	ProcessNode(scene->mRootNode, scene, glm::mat4(1.0f));

	if (meshes.empty())
	{
		boundsMin = glm::vec3(0.0f);
		boundsMax = glm::vec3(0.0f);
	}
}

void Model::ProcessNode(const aiNode* node, const aiScene* scene, const glm::mat4& parentTransform)
{
	glm::mat4 global = parentTransform * ToGlm(node->mTransformation);

	for (unsigned int i = 0; i < node->mNumMeshes; i++)
		AddMesh(scene->mMeshes[node->mMeshes[i]], scene, global);

	for (unsigned int i = 0; i < node->mNumChildren; i++)
		ProcessNode(node->mChildren[i], scene, global);
}

void Model::AddMesh(const aiMesh* mesh, const aiScene* scene, const glm::mat4& transform)
{
	// Normals transform by the inverse-transpose of the upper-left 3x3.
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

	// Import the material diffuse color as the albedo (default to light grey).
	glm::vec3 albedo(0.8f, 0.8f, 0.8f);
	if (scene->mNumMaterials > 0 && mesh->mMaterialIndex < scene->mNumMaterials)
	{
		aiColor3D c(0.8f, 0.8f, 0.8f);
		if (scene->mMaterials[mesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, c) == AI_SUCCESS)
			albedo = glm::vec3(c.r, c.g, c.b);
	}

	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	vertices.reserve(mesh->mNumVertices * 11);

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		// Bake the node transform into the position.
		glm::vec3 pos = glm::vec3(transform * glm::vec4(
			mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 1.0f));
		vertices.push_back(pos.x);
		vertices.push_back(pos.y);
		vertices.push_back(pos.z);

		boundsMin = glm::min(boundsMin, pos);
		boundsMax = glm::max(boundsMax, pos);

		// color (per-vertex; albedo uniform is what the model shader actually uses)
		vertices.push_back(0.8f);
		vertices.push_back(0.8f);
		vertices.push_back(0.8f);

		// uv
		if (mesh->mTextureCoords[0])
		{
			vertices.push_back(mesh->mTextureCoords[0][i].x);
			vertices.push_back(mesh->mTextureCoords[0][i].y);
		}
		else
		{
			vertices.push_back(0.0f);
			vertices.push_back(0.0f);
		}

		// normal (transformed, renormalized in the shader)
		if (mesh->HasNormals())
		{
			glm::vec3 n = normalMatrix * glm::vec3(
				mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			vertices.push_back(n.x);
			vertices.push_back(n.y);
			vertices.push_back(n.z);
		}
		else
		{
			vertices.push_back(0.0f);
			vertices.push_back(1.0f);
			vertices.push_back(0.0f);
		}
	}

	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	meshes.push_back(SubMesh{
		Mesh(vertices, indices, { {3, GL_FLOAT}, {3, GL_FLOAT}, {2, GL_FLOAT}, {3, GL_FLOAT} }),
		albedo });
}

float Model::ScaleToFit(float targetSize) const
{
	glm::vec3 s = Size();
	float largest = std::max(s.x, std::max(s.y, s.z));
	if (largest <= 0.0f) return 1.0f;
	return targetSize / largest;
}

void Model::Draw(Shader& shader)
{
	for (auto& sub : meshes)
	{
		shader.SetUniformValueVec3("albedo", sub.albedo);
		sub.mesh.Draw();
	}
}
