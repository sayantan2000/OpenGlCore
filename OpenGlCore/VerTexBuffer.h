#ifndef VBO_CLASS_H
#define VBO_CLASS_H

#include<glad/glad.h>

class VerTexBuffer
{
public:
	// Reference ID of the Vertex Buffer Object (0 = no owned GL object)
	GLuint ID = 0;
	// Constructor that generates a Vertex Buffer Object and links it to vertices
	VerTexBuffer(GLfloat* vertices, GLsizeiptr size);
	// Frees the GL buffer (RAII)
	~VerTexBuffer();

	// Non-copyable: copying would duplicate the raw handle and double-free it.
	VerTexBuffer(const VerTexBuffer&) = delete;
	VerTexBuffer& operator=(const VerTexBuffer&) = delete;
	// Movable: transfers ownership of the handle.
	VerTexBuffer(VerTexBuffer&& other) noexcept;
	VerTexBuffer& operator=(VerTexBuffer&& other) noexcept;

	// Binds the VBO
	void Bind();
	// Unbinds the VBO
	void Unbind();
	// Deletes the VBO (idempotent; safe to call multiple times)
	void Delete();
};

#endif