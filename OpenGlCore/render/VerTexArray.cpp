#include"VerTexArray.h"
#include"GLUtils.h"

namespace ogl {

// Constructor that generates a VAO ID
VerTexArray::VerTexArray()
{
	glGenVertexArrays(1, &ID);
}

// Links a VBO to the VAO using a certain layout
void VerTexArray::LinkAttriBute(VerTexBuffer& VBO, GLuint layout, GLuint numComponents, GLenum typr, GLsizeiptr stride, void* offset)
{
	VBO.Bind();
	glVertexAttribPointer(layout, numComponents, typr, GL_FALSE, stride, offset);
	glEnableVertexAttribArray(layout);
	VBO.Unbind();
	glCheckError();
}

// Binds the VAO
void VerTexArray::Bind()
{
	glBindVertexArray(ID);
}

// Unbinds the VAO
void VerTexArray::Unbind()
{
	glBindVertexArray(0);
}

// Deletes the VAO (idempotent)
void VerTexArray::Delete()
{
	if (ID != 0)
	{
		glDeleteVertexArrays(1, &ID);
		ID = 0;
	}
}

VerTexArray::~VerTexArray()
{
	Delete();
}

VerTexArray::VerTexArray(VerTexArray&& other) noexcept
{
	ID = other.ID;
	other.ID = 0;
}

VerTexArray& VerTexArray::operator=(VerTexArray&& other) noexcept
{
	if (this != &other)
	{
		Delete();
		ID = other.ID;
		other.ID = 0;
	}
	return *this;
}
} // namespace ogl