#include"VerTexBuffer.h"

// Constructor that generates a Vertex Buffer Object and links it to vertices
VerTexBuffer::VerTexBuffer(GLfloat* vertices, GLsizeiptr size)
{
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ARRAY_BUFFER, ID);
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

// Binds the VBO
void VerTexBuffer::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

// Unbinds the VBO
void VerTexBuffer::Unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Deletes the VBO (idempotent)
void VerTexBuffer::Delete()
{
	if (ID != 0)
	{
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}

VerTexBuffer::~VerTexBuffer()
{
	Delete();
}

VerTexBuffer::VerTexBuffer(VerTexBuffer&& other) noexcept
{
	ID = other.ID;
	other.ID = 0;
}

VerTexBuffer& VerTexBuffer::operator=(VerTexBuffer&& other) noexcept
{
	if (this != &other)
	{
		Delete();
		ID = other.ID;
		other.ID = 0;
	}
	return *this;
}