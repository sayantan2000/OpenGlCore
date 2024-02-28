#include"VerTexArray.h"

// Constructor that generates a VAO ID
VerTexArray::VerTexArray()
{
	glGenVertexArrays(1, &ID);
}

// Links a VBO to the VAO using a certain layout
void VerTexArray::LinkAttriBute(VerTexBuffer VBO, GLuint layout, GLuint numComponents, GLenum typr, GLsizeiptr stride, void* offset)
{
	VBO.Bind();
	glVertexAttribPointer(layout, numComponents, typr, GL_FALSE, stride, offset);
	glEnableVertexAttribArray(layout);
	VBO.Unbind();
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

// Deletes the VAO
void VerTexArray::Delete()
{
	glDeleteVertexArrays(1, &ID);
}