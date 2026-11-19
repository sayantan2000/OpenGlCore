#include"ElementBuffer.h"
#include"GLUtils.h"

// Constructor that generates a Elements Buffer Object and links it to indices
ElementBuffer::ElementBuffer(GLuint* indices, GLsizeiptr size)
{
	glGenBuffers(1, &ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, indices, GL_STATIC_DRAW);
	glCheckError();
}

// Binds the ElementBuffer
void ElementBuffer::Bind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ID);
}

// Unbinds the ElementBuffer
void ElementBuffer::Unbind()
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// Deletes the ElementBuffer (idempotent)
void ElementBuffer::Delete()
{
	if (ID != 0)
	{
		glDeleteBuffers(1, &ID);
		ID = 0;
	}
}

ElementBuffer::~ElementBuffer()
{
	Delete();
}

ElementBuffer::ElementBuffer(ElementBuffer&& other) noexcept
{
	ID = other.ID;
	other.ID = 0;
}

ElementBuffer& ElementBuffer::operator=(ElementBuffer&& other) noexcept
{
	if (this != &other)
	{
		Delete();
		ID = other.ID;
		other.ID = 0;
	}
	return *this;
}