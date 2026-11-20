#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace ogl {

// Simple TRS transform. Rotation is stored as Euler angles in DEGREES (X, Y, Z) for a
// familiar, editable Inspector representation; the matrix composes T * R * S.
struct Transform
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotationEuler = glm::vec3(0.0f); // degrees
	glm::vec3 scale = glm::vec3(1.0f);

	glm::mat4 Matrix() const
	{
		glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
		glm::mat4 r = glm::mat4_cast(glm::quat(glm::radians(rotationEuler)));
		glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
		return t * r * s;
	}

	void Reset()
	{
		position = glm::vec3(0.0f);
		rotationEuler = glm::vec3(0.0f);
		scale = glm::vec3(1.0f);
	}
};


} // namespace ogl
#endif
