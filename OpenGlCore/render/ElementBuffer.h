#ifndef EBO_CLASS_H
#define EBO_CLASS_H

#include<glad/glad.h>

class ElementBuffer
{
public:
	// ID reference of Elements Buffer Object (0 = no owned GL object)
	GLuint ID = 0;
	// Constructor that generates a Elements Buffer Object and links it to indices
	ElementBuffer(GLuint* indices, GLsizeiptr size);
	// Frees the GL buffer (RAII)
	~ElementBuffer();

	// Non-copyable, movable.
	ElementBuffer(const ElementBuffer&) = delete;
	ElementBuffer& operator=(const ElementBuffer&) = delete;
	ElementBuffer(ElementBuffer&& other) noexcept;
	ElementBuffer& operator=(ElementBuffer&& other) noexcept;

	// Binds the EBO
	void Bind();
	// Unbinds the EBO
	void Unbind();
	// Deletes the EBO (idempotent)
	void Delete();
};

#endif