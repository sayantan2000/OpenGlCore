#include"Texture.h"
#include"GLUtils.h"
#include<stdexcept>
#include<iostream>

Texture::Texture(const char* image, GLenum texType, GLenum slot, GLenum /*format*/, GLenum /*pixelType*/)
{
	type = texType;
	// Remember which texture unit this texture belongs to (slot is GL_TEXTURE0 + n).
	unit = slot - GL_TEXTURE0;

	int widthImg, heightImg, numColCh;
	stbi_set_flip_vertically_on_load(true); // OpenGL's origin is bottom-left
	unsigned char* bytes = stbi_load(image, &widthImg, &heightImg, &numColCh, 0);
	if (bytes == nullptr)
	{
		std::string msg = "Failed to load texture image: '";
		msg += image;
		msg += "'. Check the path / working directory.";
		std::cerr << "[TEXTURE ERROR] " << msg << std::endl;
		throw std::runtime_error(msg);
	}

	// Choose the GL format from the image's ACTUAL channel count. Previously the format was
	// forced to the caller's value (GL_RGB) regardless of the file: uploading a grayscale
	// (1-channel) map such as Roughness/AO as RGB makes GL read 3x the bytes per row, shearing
	// every row and producing a "scattered" texture. Also relax the unpack alignment so rows
	// whose byte length isn't a multiple of 4 (common for 1/3-channel images) aren't misread.
	GLenum fmt =
		(numColCh == 1) ? GL_RED :
		(numColCh == 2) ? GL_RG :
		(numColCh == 4) ? GL_RGBA : GL_RGB;

	glGenTextures(1, &ID);
	glActiveTexture(slot);
	glBindTexture(texType, ID);

	glTexParameteri(texType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(texType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(texType, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(texType, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(texType, 0, fmt, widthImg, heightImg, 0, fmt, GL_UNSIGNED_BYTE, bytes);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore default
	glGenerateMipmap(texType);

	stbi_image_free(bytes);
	glBindTexture(texType, 0);
	glCheckError();
}

void Texture::texUnit(Shader& shader, const char* uniform, GLuint unit)
{
	// Gets the location of the uniform
	GLuint texUni = glGetUniformLocation(shader.ID, uniform);
	// Shader needs to be activated before changing the value of a uniform
	shader.Activate();
	// Sets the value of the uniform
	glUniform1i(texUni, unit);
}

void Texture::Bind()
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(type, ID);
}

void Texture::Unbind()
{
	glBindTexture(type, 0);
}

void Texture::Delete()
{
	if (ID != 0)
	{
		glDeleteTextures(1, &ID);
		ID = 0;
	}
}

Texture::~Texture()
{
	Delete();
}

Texture::Texture(Texture&& other) noexcept
{
	ID = other.ID;
	type = other.type;
	unit = other.unit;
	other.ID = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
	if (this != &other)
	{
		Delete();
		ID = other.ID;
		type = other.type;
		unit = other.unit;
		other.ID = 0;
	}
	return *this;
}