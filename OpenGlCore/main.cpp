#include <iostream>
#include"glad/glad.h"
#include"GLFW/glfw3.h"


//defining vertexShader

// Vertex Shader source code
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
//Fragment Shader source code
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
"}\n\0";

int main() {
	//Initializiation of GLFW
	glfwInit();

	//Version Setup OF GLFW/OPENGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLfloat verts[] = {
		-0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower left corner
		0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f, // Lower right corner
		0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f, // Upper corner
		-0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner left
		0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f, // Inner right
		0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f // Inner down
	};

	GLuint Triangles[] = {
		0, 3, 5, // Lower left triangle
		3, 2, 4, // Upper triangle
		5, 4, 1 // Lower right triangle
	};


	//creating Window
	GLFWwindow* window = glfwCreateWindow(800, 800, "OpenGLCORE", nullptr, nullptr);

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
	glViewport(0, 0, 800, 800);

	//Assigning predifned vertex shader to opengl
	GLuint vettexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vettexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vettexShader);


	//Assigning predifned FragMent shader to opengl
	GLuint FragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragShader, 1, &fragmentShaderSource, nullptr);
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


	GLuint VAO, Vbo, Tbo;

	//Generating VertexArray
	glGenVertexArrays(1, &VAO);

	//binding vertex array
	glBindVertexArray(VAO);

	//generating vertex buffer
	glGenBuffers(1, &Vbo);
	//generating triangle buffer
	glGenBuffers(1, &Tbo);

	//binding the buffer
	glBindBuffer(GL_ARRAY_BUFFER, Vbo);

	//binding traingle buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Tbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Triangles), Triangles, GL_STATIC_DRAW);

	//Assingning VertexData To Buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Binding The Buffer
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);






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
		glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);


		//Listening GLFWEVENTS
		glfwPollEvents();
	}
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &Vbo);
	glDeleteProgram(ShaderProgram);
	glDeleteBuffers(1, &Tbo);
	//destroying window
	glfwDestroyWindow(window);


	//Terminating The GLFW 
	glfwTerminate();
	return 0;
}