#include "Model.h"
#include "GLUtils.h"
#include <stdexcept>
#include <iostream>
#include <limits>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Uploads 8-bit image data as a GL texture with mipmaps; returns the texture id.
static GLuint MakeGLTexture(const unsigned char* data, int w, int h, int channels)
{
	GLenum format = (channels == 1) ? GL_RED : (channels == 3) ? GL_RGB : GL_RGBA;
	GLuint id = 0;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	return id;
}

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

	// Remember the model's folder so external texture paths can be resolved.
	size_t slash = path.find_last_of("/\\");
	directory = (slash == std::string::npos) ? std::string(".") : path.substr(0, slash);

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

	// Import the material diffuse color (albedo) and, if present, the diffuse texture.
	glm::vec3 albedo(0.8f, 0.8f, 0.8f);
	GLuint textureID = 0;
	if (scene->mNumMaterials > 0 && mesh->mMaterialIndex < scene->mNumMaterials)
	{
		const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

		aiColor3D c(0.8f, 0.8f, 0.8f);
		if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, c) == AI_SUCCESS)
			albedo = glm::vec3(c.r, c.g, c.b);

		if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
		{
			aiString texPath;
			if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
			{
				// Embedded (either "*index" or resolvable via GetEmbeddedTexture), else file.
				const aiTexture* embedded = scene->GetEmbeddedTexture(texPath.C_Str());
				if (embedded)
					textureID = LoadEmbeddedTexture(embedded);
				else
					textureID = LoadTextureFromFile(directory + "/" + texPath.C_Str());
			}
		}
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
		albedo, textureID });
}

GLuint Model::LoadTextureFromFile(const std::string& path)
{
	stbi_set_flip_vertically_on_load(true);
	int w = 0, h = 0, n = 0;
	unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, 0);
	if (!data)
	{
		std::cerr << "[MODEL WARN] could not load texture: " << path << std::endl;
		return 0;
	}
	GLuint id = MakeGLTexture(data, w, h, n);
	stbi_image_free(data);
	return id;
}

GLuint Model::LoadEmbeddedTexture(const aiTexture* tex)
{
	// mHeight == 0 => compressed (png/jpg bytes in pcData, mWidth = byte count).
	if (tex->mHeight == 0)
	{
		stbi_set_flip_vertically_on_load(false);
		int w = 0, h = 0, n = 0;
		unsigned char* data = stbi_load_from_memory(
			reinterpret_cast<const unsigned char*>(tex->pcData), (int)tex->mWidth, &w, &h, &n, 0);
		if (!data)
		{
			std::cerr << "[MODEL WARN] could not decode embedded texture" << std::endl;
			return 0;
		}
		GLuint id = MakeGLTexture(data, w, h, n);
		stbi_image_free(data);
		return id;
	}
	// Uncompressed: aiTexel array is BGRA; repack to RGBA.
	int count = tex->mWidth * tex->mHeight;
	std::vector<unsigned char> rgba((size_t)count * 4);
	for (int i = 0; i < count; i++)
	{
		rgba[i * 4 + 0] = tex->pcData[i].r;
		rgba[i * 4 + 1] = tex->pcData[i].g;
		rgba[i * 4 + 2] = tex->pcData[i].b;
		rgba[i * 4 + 3] = tex->pcData[i].a;
	}
	return MakeGLTexture(rgba.data(), tex->mWidth, tex->mHeight, 4);
}

Model::~Model()
{
	for (auto& sub : meshes)
		if (sub.textureID != 0)
			glDeleteTextures(1, &sub.textureID);
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
	GLint hasTexLoc = glGetUniformLocation(shader.ID, "hasTexture");
	for (auto& sub : meshes)
	{
		if (sub.textureID != 0)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, sub.textureID);
			glUniform1i(hasTexLoc, 1);
		}
		else
		{
			shader.SetUniformValueVec3("albedo", sub.albedo);
			glUniform1i(hasTexLoc, 0);
		}
		sub.mesh.Draw();
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}
