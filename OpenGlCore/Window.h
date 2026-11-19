#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Owns GLFW init, the window + GL context, and GLAD loading. Constructing it leaves a
// current OpenGL 3.3 core context ready for use; destroying it tears everything down.
class Window
{
public:
	Window(int width, int height, const char* title, bool fullscreen);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	bool ShouldClose() const;
	void SwapBuffers() const;
	void PollEvents() const;
	void SetTitle(const char* title) const;

	GLFWwindow* Handle() const { return window; }
	int Width() const { return width; }
	int Height() const { return height; }

private:
	GLFWwindow* window = nullptr;
	int width = 0;
	int height = 0;
};

#endif
