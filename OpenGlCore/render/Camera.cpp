#include"Camera.h"

namespace ogl {



Camera::Camera(int width, int height, glm::vec3 position)
{
	Camera::width = width;
	Camera::height = height;
	Position = position;
}

void Camera::updateMatrix(float FOVdeg, float nearPlane, float farPlane)
{
	// Initializes matrices since otherwise they will be the null matrix
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 projection = glm::mat4(1.0f);

	// Makes camera look in the right direction from the right position
	view = glm::lookAt(Position, Position + Orientation, Up);
	// Adds perspective to the scene
	projection = glm::perspective(glm::radians(FOVdeg), (float)width / height, nearPlane, farPlane);

	// Sets new camera matrix
	cameraMatrix = projection * view;
}

glm::mat4 Camera::GetViewMatrix() const
{
	return glm::lookAt(Position, Position + Orientation, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float FOVdeg, float nearPlane, float farPlane) const
{
	return glm::perspective(glm::radians(FOVdeg), (float)width / (float)height, nearPlane, farPlane);
}

void Camera::Matrix(Shader& shader, const char* uniform)
{
	// Exports camera matrix
	glUniformMatrix4fv(glGetUniformLocation(shader.ID, uniform), 1, GL_FALSE, glm::value_ptr(cameraMatrix));
}



void Camera::Inputs(GLFWwindow* window, float tick)
{
	// Unity/Unreal-style flythrough: navigation (WASD/space/ctrl movement + mouse look) is
	// active only while the RIGHT mouse button is held. This leaves the LEFT button free for
	// editor object selection and gizmo dragging, and leaves W/E/R free as tool shortcuts.
	const bool navigating = (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

	if (!navigating)
	{
		// Release the cursor and reset the look state so the next drag doesn't jump.
		if (!firstClick)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			firstClick = true;
		}
		return;
	}

	// Sprint check first so this frame's movement uses the correct speed. `speed` is in
	// units/second; multiplying by `tick` (delta-time) makes movement frame-rate independent.
	speed = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ? 8.0f : 3.0f;
	float velocity = speed * tick;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) Position += velocity * Orientation;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) Position += velocity * -Orientation;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) Position += velocity * -glm::normalize(glm::cross(Orientation, Up));
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) Position += velocity * glm::normalize(glm::cross(Orientation, Up));
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) Position += velocity * Up;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) Position += velocity * -Up;

	// Mouse look (right button held).
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	if (firstClick)
	{
		glfwSetCursorPos(window, (width / 2), (height / 2));
		firstClick = false;
	}

	double mouseX, mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);

	float rotX = sensitivity * (float)(mouseY - (height / 2)) / height;
	float rotY = sensitivity * (float)(mouseX - (width / 2)) / width;

	glm::vec3 newOrientation = glm::rotate(Orientation, glm::radians(-rotX), glm::normalize(glm::cross(Orientation, Up)));
	if (abs(glm::angle(newOrientation, Up) - glm::radians(90.0f)) <= glm::radians(85.0f))
		Orientation = newOrientation;
	Orientation = glm::rotate(Orientation, glm::radians(-rotY), Up);

	glfwSetCursorPos(window, (width / 2), (height / 2));
}
} // namespace ogl