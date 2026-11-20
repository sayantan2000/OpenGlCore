#include "Input.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstring>

namespace ogl {

void Input::Init(GLFWwindow* w)
{
	window = w;
	std::memset(cur, 0, sizeof(cur));
	std::memset(prev, 0, sizeof(prev));
	firstUpdate = true;
}

bool Input::KeyNow(int key) const
{
	if (!window || key < 0 || key >= kMaxKeys) return false;
	return glfwGetKey(window, key) == GLFW_PRESS;
}

void Input::Update()
{
	if (!window) return;

	std::memcpy(prev, cur, sizeof(cur));
	for (int k = 0; k < kMaxKeys; ++k)
		cur[k] = glfwGetKey(window, k) == GLFW_PRESS;

	double mx, my;
	glfwGetCursorPos(window, &mx, &my);
	glm::vec2 now((float)mx, (float)my);
	mouseDelta = firstUpdate ? glm::vec2(0.0f) : (now - mousePos);
	mousePos = now;
	firstUpdate = false;
}

bool Input::GetKey(int key) const { return (key >= 0 && key < kMaxKeys) && cur[key]; }
bool Input::GetKeyDown(int key) const { return (key >= 0 && key < kMaxKeys) && cur[key] && !prev[key]; }
bool Input::GetKeyUp(int key) const { return (key >= 0 && key < kMaxKeys) && !cur[key] && prev[key]; }

bool Input::GetMouseButton(int button) const
{
	return window && glfwGetMouseButton(window, button) == GLFW_PRESS;
}

float Input::GetAxis(const char* axis) const
{
	if (std::strcmp(axis, "Horizontal") == 0)
	{
		float v = 0.0f;
		if (GetKey(GLFW_KEY_D) || GetKey(GLFW_KEY_RIGHT)) v += 1.0f;
		if (GetKey(GLFW_KEY_A) || GetKey(GLFW_KEY_LEFT))  v -= 1.0f;
		return v;
	}
	if (std::strcmp(axis, "Vertical") == 0)
	{
		float v = 0.0f;
		if (GetKey(GLFW_KEY_W) || GetKey(GLFW_KEY_UP))    v += 1.0f;
		if (GetKey(GLFW_KEY_S) || GetKey(GLFW_KEY_DOWN))  v -= 1.0f;
		return v;
	}
	return 0.0f;
}

} // namespace ogl