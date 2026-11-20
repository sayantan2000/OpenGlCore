#include"Mesh.h"

namespace ogl {

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

	// Retain a CPU copy of positions + indices for physics mesh cooking. Position is
	// attribute 0 (first 3 floats of each vertex); strideFloats is the per-vertex span.
	GLuint strideFloats = 0;
	for (const auto& attr : layout) strideFloats += attr.numComponents;
	if (strideFloats >= 3 && !vertices.empty())
	{
		cpuPositions.reserve(vertices.size() / strideFloats);
		for (size_t i = 0; i + 2 < vertices.size(); i += strideFloats)
			cpuPositions.emplace_back(vertices[i], vertices[i + 1], vertices[i + 2]);
	}
	cpuIndices = indices;

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

} // namespace ogl