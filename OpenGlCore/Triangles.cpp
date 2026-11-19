#include"Triangles.h"
#include"GLUtils.h"

// Constructor that generates a Elements Buffer Object and links it to indices
Triangle::Triangle(GLuint* indices, GLsizeiptr size)
{
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	glCheckError();
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

// Deletes the Triangle (idempotent)
void Triangle::Delete()
{
	if (ID != 0)
	{
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}

Triangle::~Triangle()
{
	Delete();
}

Triangle::Triangle(Triangle&& other) noexcept
{
	ID = other.ID;
	other.ID = 0;
}

Triangle& Triangle::operator=(Triangle&& other) noexcept
{
	if (this != &other)
	{
		Delete();
		ID = other.ID;
		other.ID = 0;
	}
	return *this;
}