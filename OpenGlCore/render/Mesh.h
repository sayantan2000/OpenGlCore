#ifndef MESH_CLASS_H
#define MESH_CLASS_H

#include<vector>
#include<glad/glad.h>
#include"VerTexArray.h"
#include"VerTexBuffer.h"
#include"ElementBuffer.h"

namespace ogl {

// One attribute in a vertex layout, e.g. { 3, GL_FLOAT } for a vec3.
// Components are assumed to be 4 bytes each (GLfloat / GLuint), which covers this project.
struct VertexAttribute
{
	GLuint numComponents;
	GLenum type;
};

// Owns a VAO + VBO + EBO for one drawable mesh and wires up the attribute layout.
// Non-copyable (owns GL handles), movable. Thin by design: it bundles the buffer
// plumbing that was duplicated in main; shader/texture binding stays with the caller.
class Mesh
{
public:
	Mesh(const std::vector<GLfloat>& vertices,
	     const std::vector<GLuint>& indices,
	     const std::vector<VertexAttribute>& layout);

	// Binds the VAO and issues the indexed draw call. Caller binds shader/texture first.
	void Draw();

	GLsizei IndexCount() const { return indexCount; }

private:
	VerTexArray vao;      // declared first -> constructed first (default ctor gens the VAO)
	VerTexBuffer vbo;
	ElementBuffer ebo;
	GLsizei indexCount = 0;
};

} // namespace ogl
#endif
