#ifndef VAO_CLASS_H
#define VAO_CLASS_H

#include<glad/glad.h>
#include"VerTexBuffer.h"

class VerTexArray
{
public:
	// ID reference for the Vertex Array Object
	GLuint ID;
	// Constructor that generates a VAO ID
	VerTexArray();

	// Links a VBO to the VAO using a certain layout
	void LinkAttriBute(VerTexBuffer VBO, GLuint layout, GLuint numComponents, GLenum typr, GLsizeiptr stride, void* offset);
	// Binds the VAO
	void Bind();
	// Unbinds the VAO
	void Unbind();
	// Deletes the VAO
	void Delete();
};
#endif