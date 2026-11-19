#include "Model.h"
#include <stdexcept>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Builds one project Mesh (11-float layout) from an Assimp mesh.
static Mesh BuildMesh(const aiMesh* mesh)
{
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	vertices.reserve(mesh->mNumVertices * 11);

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		// position
		vertices.push_back(mesh->mVertices[i].x);
		vertices.push_back(mesh->mVertices[i].y);
		vertices.push_back(mesh->mVertices[i].z);
		// color (no per-vertex color from most models: use a neutral light grey)
		vertices.push_back(0.8f);
		vertices.push_back(0.8f);
		vertices.push_back(0.8f);
		// uv (0,0 when the model has no texture coordinates)
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
		// normal
		if (mesh->HasNormals())
		{
			vertices.push_back(mesh->mNormals[i].x);
			vertices.push_back(mesh->mNormals[i].y);
			vertices.push_back(mesh->mNormals[i].z);
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

	return Mesh(vertices, indices,
		{ {3, GL_FLOAT}, {3, GL_FLOAT}, {2, GL_FLOAT}, {3, GL_FLOAT} });
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

	// Flatten every mesh referenced in the scene (node hierarchy transforms are not
	// applied here; sufficient for single-mesh models like Cube.fbx).
	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
		meshes.push_back(BuildMesh(scene->mMeshes[i]));
}

void Model::Draw()
{
	for (auto& mesh : meshes)
		mesh.Draw();
}
