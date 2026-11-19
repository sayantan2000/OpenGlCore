#include"Mesh.h"

Mesh::Mesh(const std::vector<GLfloat>& vertices,
           const std::vector<GLuint>& indices,
           const std::vector<VertexAttribute>& layout)
	: vbo(const_cast<GLfloat*>(vertices.data()), (GLsizeiptr)(vertices.size() * sizeof(GLfloat))),
	  ebo(const_cast<GLuint*>(indices.data()), (GLsizeiptr)(indices.size() * sizeof(GLuint)))
{
	indexCount = (GLsizei)indices.size();

	// Total vertex stride in bytes (every component assumed 4 bytes).
	GLsizei stride = 0;
	for (const auto& attr : layout)
		stride += (GLsizei)(attr.numComponents * sizeof(GLfloat));

	vao.Bind();
	// Bind the EBO while the VAO is bound so the VAO records the element buffer binding.
	ebo.Bind();

	size_t offset = 0;
	for (GLuint i = 0; i < (GLuint)layout.size(); ++i)
	{
		vao.LinkAttriBute(vbo, i, layout[i].numComponents, layout[i].type, stride, (void*)offset);
		offset += layout[i].numComponents * sizeof(GLfloat);
	}

	// Unbind VAO first so it keeps its EBO association, then the buffers.
	vao.Unbind();
	vbo.Unbind();
	ebo.Unbind();
}

void Mesh::Draw()
{
	vao.Bind();
	glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}
