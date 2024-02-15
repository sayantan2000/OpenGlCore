#include <iostream>
#include"glad/glad.h"
#include"GLFW/glfw3.h"


//defining vertexShader

const char* VertexShadersrc = "#version 330 core\n"
"layout (location =0) in vec3 a_Pos;\n"
"void main()\n"
"{\n"
"   gl_Position=vec4(aPos.x,aPos.y,aPos.z,1.0);\n"
"}\0";

const char* FragShadersrc = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor=vec4(0.8f,0.3f,0.02f,1.0f);\n"
"}\n\0";

int main() {
	//Initializiation of GLFW
	glfwInit();

	//Version Setup OF GLFW/OPENGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLfloat verts[] = {
		 -0.5f,-0.5f * float(sqrt(3)) / 3,0.0f,
		 0.5f,-0.5f * float(sqrt(3)) / 3,0.0f,
		 0.0f,0.5f * float(sqrt(3)) * 2 / 3,0.0f
	};


	//creating Window
	GLFWwindow* window = glfwCreateWindow(640, 360, "OpenGLCORE", nullptr, nullptr);

	if (window == nullptr) {
		std::wcout << "failed to create Window" << std::endl;
		glfwTerminate();
		return -1;
	}

	//make the newly created window as current active context
	glfwMakeContextCurrent(window);

	//loading glad
	gladLoadGL();

	//setting the viewport cordinates
	glViewport(0, 0, 640, 360);

	//Assigning predifned vertex shader to opengl
	GLuint vettexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vettexShader, 1, &VertexShadersrc, nullptr);
	glCompileShader(vettexShader);


	//Assigning predifned FragMent shader to opengl
	GLuint FragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragShader, 1, &FragShadersrc, nullptr);
	glCompileShader(FragShader);


	//create ShaderProgram
	GLuint ShaderProgram = glCreateProgram();
	//Attaching the shader to program 
	glAttachShader(ShaderProgram, vettexShader);
	glAttachShader(ShaderProgram, FragShader);

	//Linking The ShaderProgram to opengl
	glLinkProgram(ShaderProgram);

	//Deleting The Shader
	glDeleteShader(vettexShader);
	glDeleteShader(FragShader);


	GLuint VAO, Vbo;

	//Generating VertexArray
	glGenVertexArrays(1, &VAO);

	//binding vertex array
	glBindVertexArray(VAO);

	//generating vertex buffer
	glGenBuffers(1, &Vbo);

	//binding the buffer
	glBindBuffer(GL_ARRAY_BUFFER, Vbo);

	//Assingning VertexData To Buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Binding The Buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);






	//setting the screenspace colors
	glClearColor(0.07f, 0.13f, 0.17f, 1.00f);

	//Assigning the back buffer to new color
	glClear(GL_COLOR_BUFFER_BIT);
	glfwSwapBuffers(window);
	//listening window events
	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.07f, 0.13f, 0.17f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(ShaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glfwSwapBuffers(window);


		//Listening GLFWEVENTS
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &Vbo);
	glDeleteProgram(ShaderProgram);
	//destroying window
	glfwDestroyWindow(window);


	//Terminating The GLFW 
	glfwTerminate();
	return 0;
}