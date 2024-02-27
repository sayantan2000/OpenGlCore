#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include"shaderClass.h"
#include"VerTexArray.h"
#include"VerTexBuffer.h"
#include"Triangles.h"
#include "Texture.h";
#include "Camera.h";




// Vertices coordinates
GLfloat vertices[] =
{
	//trinagles
	//// cordinates										//colors                       
	//-0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,			0.3f,0.2f,.5f,											 // Lower left corner
	//0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,				0.3f,0.1f,.5f,											 // Lower right corner
	//0.0f, 0.5f * float(sqrt(3)) * 2 / 3, 0.0f,			0.3f,0.7f,.4f,											 // Upper corner
	//-0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f,			0.3f,0.5f,.2f,											 // Inner left
	//0.5f / 2, 0.5f * float(sqrt(3)) / 6, 0.0f,			0.3f,0.2f,.5f,											 // Inner right
	//0.0f, -0.5f * float(sqrt(3)) / 3, 0.0f															 // Inner down


	//square
	 // COORDINATES / COLORS / TexCoord  //
	-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	0.0f, 0.0f,
	-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	5.0f, 0.0f,
	 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	0.0f, 0.0f,
	 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	5.0f, 0.0f,
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	2.5f, 5.0f
};

// Indices for vertices order
GLuint indices[] =
{
	//traingle indices
	//0, 3, 5, // Lower left triangle
	//3, 2, 4, // Lower right triangle
	//5, 4, 1 // Upper triangle

	//square indices
	0, 1, 2,
	0, 2, 3,
	0, 1, 4,
	1, 2, 4,
	2, 3, 4,
	3, 0, 4

};

float oldTimeSinceStart = 0;
const int HEIGHT = 1080, WIDTH = 1920;

int main()
{
	float currentFrame = glfwGetTime();
	float lastFrame = currentFrame;
	float deltaTime = 0.00f;

	float a = 0;
	float speed = 0.6;

	a += deltaTime * speed;
	stbi_set_flip_vertically_on_load(true);
	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "YoutubeOpenGL", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, WIDTH, HEIGHT);



	// Generates Shader object using shaders defualt.vert and default.frag
	Shader shaderProgram("./Shader/default.vert", "./SHader/default.frag");



	// Generates Vertex Array Object and binds it
	VerTexArray VAO1;
	VAO1.Bind();

	// Generates Vertex Buffer Object and links it to vertices
	VerTexBuffer VBO1(vertices, sizeof(vertices));
	// Generates Element Buffer Object and links it to indices
	Triangle EBO1(indices, sizeof(indices));

	// Links VBO to VAO
	VAO1.LinkVBO(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkVBO(VBO1, 1, 3, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkVBO(VBO1, 2, 2, GL_FLOAT, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	// Unbind all to prevent accidentally modifying them
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	std::string TexPath("./Images/Brick.jpg");

	Texture texture(TexPath.c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
	texture.texUnit(shaderProgram, "tex0", 0);



	//getting the id of the uniform variable from active shader
	shaderProgram.Activate();
	GLuint UniformID = glGetUniformLocation(shaderProgram.ID, "scale");
	GLuint UnformModel = glGetUniformLocation(shaderProgram.ID, "model");

	Camera camera(WIDTH, HEIGHT, glm::vec3(0.0f, .5f, 2.0f));

	float rotation = 0.0f;
	double prevTime = glfwGetTime();
	glEnable(GL_DEPTH_TEST);
	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		double crntTime = glfwGetTime();
		if (crntTime - prevTime >= 1 / 60)
		{
			rotation += 0.5f;
			prevTime = crntTime;
		}
		glm::mat4 model = glm::mat4(1.00f);
		model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.00f));

		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		a += deltaTime * speed;

		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Tell OpenGL which Shader Program we want to use
		shaderProgram.Activate();
		camera.updateMatrix(45.0f, 0.1f, 100.00f);
		camera.Inputs(window, deltaTime);
		glUniform1f(UniformID, 1.00f);
		glUniformMatrix4fv(UnformModel, 1, GL_FALSE, glm::value_ptr(model));
		// Bind the VAO so OpenGL knows to use it
		VAO1.Bind();
		texture.Bind();
		// Draw primitives, number of indices, datatype of indices, index of indices
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(int), GL_UNSIGNED_INT, 0);
		camera.Matrix(shaderProgram, "camMatrix");
		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}



	// Delete all the objects we've created
	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();

	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}