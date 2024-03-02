#pragma once
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
class Text
{
private:
	const char* m_Text = nullptr;
public:
	Text(GLFWwindow* window,const char* a_tex, glm::vec3& color,float X = 10.00f, float y = 50.00f, float scake = 1.00f );
	void Draw();
};

