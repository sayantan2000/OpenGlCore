#include "LineRenderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

LineRenderer::LineRenderer()
{
	shader = std::make_unique<Shader>("./Shader/line.vert", "./Shader/line.frag");
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glBindVertexArray(0);
}

LineRenderer::~LineRenderer()
{
	if (vbo) glDeleteBuffers(1, &vbo);
	if (vao) glDeleteVertexArrays(1, &vao);
}

void LineRenderer::AddLine(const glm::vec3& a, const glm::vec3& b)
{
	verts.push_back(a);
	verts.push_back(b);
}

void LineRenderer::AddBox(const glm::vec3& mn, const glm::vec3& mx, const glm::mat4& model)
{
	glm::vec3 c[8] = {
		{mn.x, mn.y, mn.z}, {mx.x, mn.y, mn.z}, {mx.x, mx.y, mn.z}, {mn.x, mx.y, mn.z},
		{mn.x, mn.y, mx.z}, {mx.x, mn.y, mx.z}, {mx.x, mx.y, mx.z}, {mn.x, mx.y, mx.z},
	};
	for (glm::vec3& p : c) p = glm::vec3(model * glm::vec4(p, 1.0f));
	int edges[12][2] = { {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7} };
	for (auto& e : edges) AddLine(c[e[0]], c[e[1]]);
}

void LineRenderer::AddCircle(const glm::vec3& center, const glm::vec3& axisU, const glm::vec3& axisV,
	float radius, int segments)
{
	const float TWO_PI = 6.28318530718f;
	glm::vec3 prev = center + axisU * radius;
	for (int i = 1; i <= segments; ++i)
	{
		float a = (float)i / segments * TWO_PI;
		glm::vec3 p = center + (axisU * std::cos(a) + axisV * std::sin(a)) * radius;
		AddLine(prev, p);
		prev = p;
	}
}

void LineRenderer::Flush(const glm::mat4& viewProj, const glm::vec3& color, float lineWidth)
{
	if (verts.empty()) return;

	shader->Activate();
	glUniformMatrix4fv(glGetUniformLocation(shader->ID, "u_viewProj"), 1, GL_FALSE, glm::value_ptr(viewProj));
	glUniform3fv(glGetUniformLocation(shader->ID, "u_color"), 1, glm::value_ptr(color));

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	const size_t bytes = verts.size() * sizeof(glm::vec3);
	if (verts.size() > vboCapacity)
	{
		glBufferData(GL_ARRAY_BUFFER, bytes, verts.data(), GL_DYNAMIC_DRAW);
		vboCapacity = verts.size();
	}
	else
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, verts.data());
	}

	glLineWidth(lineWidth);
	glDrawArrays(GL_LINES, 0, (GLsizei)verts.size());
	glBindVertexArray(0);

	verts.clear();
}
