#ifndef TEXTURE_CLASS_H
#define TEXTURE_CLASS_H

#include<glad/glad.h>
#include<stb/stb.h>

#include"shaderClass.h"

namespace ogl {

class Texture
{
public:
	GLuint ID = 0;
	GLenum type = 0;
	GLuint unit = 0;
	Texture(const char* image, GLenum texType, GLenum slot, GLenum format, GLenum pixelType);
	// Frees the GL texture (RAII)
	~Texture();

	// Non-copyable, movable.
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	// Assigns a texture unit to a texture
	void texUnit(Shader& shader, const char* uniform, GLuint unit);
	// Binds a texture
	void Bind();
	// Unbinds a texture
	void Unbind();
	// Deletes a texture (idempotent)
	void Delete();
};

} // namespace ogl
#endif