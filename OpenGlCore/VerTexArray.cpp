#include"VerTexArray.h"

// Constructor that generates a VAO ID
VerTexArray::VerTexArray()
{
	glGenVertexArrays(1, &ID);
}

// Links a VBO to the VAO using a certain layout
void VerTexArray::LinkVBO(VerTexBuffer VBO, GLuint layout)
{
	VBO.Bind();
	glVertexAttribPointer(layout, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
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