#include"Triangles.h"

// Constructor that generates a Elements Buffer Object and links it to indices
Triangle::Triangle(GLuint* indices, GLsizeiptr size)
{
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
}

// Binds the Triangle
void Triangle::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

// Unbinds the Triangle
void Triangle::Unbind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Deletes the Triangle
void Triangle::Delete()
{
	glDeleteBuffers(1, &ID);
}