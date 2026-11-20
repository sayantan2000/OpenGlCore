#ifndef VAO_CLASS_H
#define VAO_CLASS_H

#include<glad/glad.h>
#include"VerTexBuffer.h"

namespace ogl {

class VerTexArray
{
public:
	// ID reference for the Vertex Array Object (0 = no owned GL object)
	GLuint ID = 0;
	// Constructor that generates a VAO ID
	VerTexArray();
	// Frees the GL vertex array (RAII)
	~VerTexArray();

	// Non-copyable, movable.
	VerTexArray(const VerTexArray&) = delete;
	VerTexArray& operator=(const VerTexArray&) = delete;
	VerTexArray(VerTexArray&& other) noexcept;
	VerTexArray& operator=(VerTexArray&& other) noexcept;

	// Links a VBO to the VAO using a certain layout (VBO passed by reference: no copy)
	void LinkAttriBute(VerTexBuffer& VBO, GLuint layout, GLuint numComponents, GLenum typr, GLsizeiptr stride, void* offset);
	// Binds the VAO
	void Bind();
	// Unbinds the VAO
	void Unbind();
	// Deletes the VAO (idempotent)
	void Delete();
};

} // namespace ogl
#endif