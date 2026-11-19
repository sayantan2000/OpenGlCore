#include "Window.h"
#include <stdexcept>

Window::Window(int width, int height, const char* title, bool fullscreen)
{
	if (!glfwInit())
		throw std::runtime_error("Failed to initialize GLFW");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = nullptr;
	if (fullscreen)
	{
		GLFWmonitor* primary = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primary);
		if (mode != nullptr)
		{
			width = mode->width;
			height = mode->height;
		}
		monitor = primary;
	}

	window = glfwCreateWindow(width, height, title, monitor, nullptr);
	if (window == nullptr)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL())
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		throw std::runtime_error("Failed to initialize GLAD");
	}

	this->width = width;
	this->height = height;
	glViewport(0, 0, width, height);
}

Window::~Window()
{
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();
}

bool Window::ShouldClose() const { return glfwWindowShouldClose(window); }
void Window::SwapBuffers() const { glfwSwapBuffers(window); }
void Window::PollEvents() const { glfwPollEvents(); }
void Window::SetTitle(const char* title) const { glfwSetWindowTitle(window, title); }
