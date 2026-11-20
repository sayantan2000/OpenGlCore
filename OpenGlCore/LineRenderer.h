#ifndef LINE_RENDERER_H
#define LINE_RENDERER_H

#include <vector>
#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "shaderClass.h"

// Minimal immediate-mode line drawer for editor overlays (light gizmos, rectangle-light
// outlines, selection wireframes). Accumulate world-space segments, then Flush() with the
// current view-projection. Owns a dynamic VBO + VAO and a small line shader (RAII).
class LineRenderer
{
public:
	LineRenderer();
	~LineRenderer();
	LineRenderer(const LineRenderer&) = delete;
	LineRenderer& operator=(const LineRenderer&) = delete;

	void AddLine(const glm::vec3& a, const glm::vec3& b);
	void AddBox(const glm::vec3& min, const glm::vec3& max, const glm::mat4& model);
	void AddCircle(const glm::vec3& center, const glm::vec3& axisU, const glm::vec3& axisV,
		float radius, int segments = 32);

	// Uploads accumulated segments and draws them in `color`, then clears the buffer.
	void Flush(const glm::mat4& viewProj, const glm::vec3& color, float lineWidth = 1.5f);
	void Clear() { verts.clear(); }

private:
	std::vector<glm::vec3> verts;
	GLuint vao = 0, vbo = 0;
	size_t vboCapacity = 0;
	std::unique_ptr<Shader> shader;
};

#endif
