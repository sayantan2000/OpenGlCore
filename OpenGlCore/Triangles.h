#ifndef EBO_CLASS_H
#define EBO_CLASS_H

#include<glad/glad.h>

class Triangle
{
public:
	// ID reference of Elements Buffer Object (0 = no owned GL object)
	GLuint ID = 0;
	// Constructor that generates a Elements Buffer Object and links it to indices
	Triangle(GLuint* indices, GLsizeiptr size);
	// Frees the GL buffer (RAII)
	~Triangle();

	// Non-copyable, movable.
	Triangle(const Triangle&) = delete;
	Triangle& operator=(const Triangle&) = delete;
	Triangle(Triangle&& other) noexcept;
	Triangle& operator=(Triangle&& other) noexcept;

	// Binds the EBO
	void Bind();
	// Unbinds the EBO
	void Unbind();
	// Deletes the EBO (idempotent)
	void Delete();
};

#endif