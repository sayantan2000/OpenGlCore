#include<Windows.h>
#include <WinUser.h>
#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include"shaderClass.h"
#include"VerTexArray.h"
#include"VerTexBuffer.h"
#include"Triangles.h"
#include "Texture.h";
#include "Camera.h";
#include "algorithm";





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
	//     COORDINATES     /        COLORS          /    TexCoord   /        NORMALS       //
	-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,      0.0f, -1.0f, 0.0f, // Bottom side
	-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 5.0f,      0.0f, -1.0f, 0.0f, // Bottom side
	 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 5.0f,      0.0f, -1.0f, 0.0f, // Bottom side
	 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, -1.0f, 0.0f, // Bottom side

	-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,     -0.8f, 0.5f,  0.0f, // Left Side
	-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,     -0.8f, 0.5f,  0.0f, // Left Side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,     -0.8f, 0.5f,  0.0f, // Left Side

	-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, 0.5f, -0.8f, // Non-facing side
	 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 0.0f,      0.0f, 0.5f, -0.8f, // Non-facing side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.0f, 0.5f, -0.8f, // Non-facing side

	 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 0.0f,      0.8f, 0.5f,  0.0f, // Right side
	 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.8f, 0.5f,  0.0f, // Right side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.8f, 0.5f,  0.0f, // Right side

	 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, 0.5f,  0.8f, // Facing side
	-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,      0.0f, 0.5f,  0.8f, // Facing side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.0f, 0.5f,  0.8f  // Facing side
};
GLfloat lightVertices[] =
{ //     COORDINATES     //


	-0.5f, 0.0f,  0.5f,
	-0.5f, 0.0f, -0.5f,
	 0.5f, 0.0f, -0.5f,
	 0.5f, 0.0f,  0.5f,
	 0.0f, 0.8f,  0.0f,




};

// Indices for vertices order
GLuint indices[] =
{
	//traingle indices
	//0, 3, 5, // Lower left triangle
	//3, 2, 4, // Lower right triangle
	//5, 4, 1 // Upper triangle

	//square indices
	0, 1, 2, // Bottom side
	0, 2, 3, // Bottom side
	4, 6, 5, // Left side
	7, 9, 8, // Non-facing side
	10, 12, 11, // Right side
	13, 15, 14 // Facing side

};

GLuint lightIndices[] =
{
	0, 1, 2,
	0, 2, 3,
	0, 1, 4,
	1, 2, 4,
	2, 3, 4,
	3, 0, 4
};

float oldTimeSinceStart = 0;
int HEIGHT = 1080, WIDTH = 1920;

int main()
{
	HEIGHT = GetSystemMetrics(SM_CYSCREEN);
	WIDTH = GetSystemMetrics(SM_CXSCREEN);
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
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGLCore", glfwGetPrimaryMonitor(), nullptr);
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
	VAO1.LinkVBO(VBO1, 0, 3, GL_FLOAT, 11 * sizeof(float), (void*)0);
	VAO1.LinkVBO(VBO1, 1, 3, GL_FLOAT, 11 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkVBO(VBO1, 2, 2, GL_FLOAT, 11 * sizeof(float), (void*)(6 * sizeof(float)));
	VAO1.LinkVBO(VBO1, 3, 3, GL_FLOAT, 11 * sizeof(float), (void*)(8 * sizeof(float)));
	// Unbind all to prevent accidentally modifying them
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	Shader lightShader("./Shader/Triangle.vert", "./Shader/Triangle.frag");
	// Generates Vertex Array Object and binds it
	VerTexArray lightVAO;
	lightVAO.Bind();
	// Generates Vertex Buffer Object and links it to vertices
	VerTexBuffer lightVBO(lightVertices, sizeof(lightVertices));
	// Generates Element Buffer Object and links it to indices
	Triangle lightEBO(lightIndices, sizeof(lightIndices));
	// Links VBO attributes such as coordinates and colors to VAO
	lightVAO.LinkVBO(lightVBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
	// Unbind all to prevent accidentally modifying them
	lightVAO.Unbind();
	lightVBO.Unbind();
	lightEBO.Unbind();

	glm::vec3 lightPos = glm::vec3(2.5f, 2.00f, 0.00f);
	glm::mat4 lightModel = glm::mat4(1.0f);
	lightModel = glm::translate(lightModel, lightPos);

	std::string TexPath("./Images/Brick.jpg");

	Texture texture(TexPath.c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGB, GL_UNSIGNED_BYTE);
	texture.texUnit(shaderProgram, "tex0", 0);



	//getting the id of the uniform variable from active shader
	shaderProgram.Activate();
	shaderProgram.SetUniformValueVec3("lightPos", lightPos);
	shaderProgram.SetUniformValueVec4("LightColor", glm::vec4(1.0f));
	lightShader.Activate();
	lightShader.SeUniFormValueMattrix("model", lightModel);
	lightShader.SetUniformValueVec4("LightColor", glm::vec4(1.00f));




	Camera camera(WIDTH, HEIGHT, glm::vec3(0.0f, .5f, 2.0f));

	float rotation = 0.0f;
	double prevTime = glfwGetTime();
	glEnable(GL_DEPTH_TEST);
	glm::mat4 model = glm::mat4(1.00f);

	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		std::wcout << "DeltaTime :-" << deltaTime << "\n" << "\0";
		double crntTime = glfwGetTime();
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		a += deltaTime * speed;
		if (crntTime - prevTime >= 1 / 60)
		{
			rotation += deltaTime;
			rotation = std::clamp(rotation, 0.0f, 1.0f);
			prevTime = crntTime;
		}
		//model = glm::mat4(1.00f);
		model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.00f));
		//lightModel = glm::rotate(lightModel, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.00f));


		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lightShader.Activate();
		// Export the camMatrix to the Vertex Shader of the light cube
		camera.Matrix(lightShader, "camMatrix");
		// Bind the VAO so OpenGL knows to use it
		lightVAO.Bind();
		lightShader.SeUniFormValueMattrix("model", lightModel);
		lightShader.SetUniformValueF("_Time", (float)glfwGetTime());
		// Draw primitives, number of indices, datatype of indices, index of indices
		glDrawElements(GL_TRIANGLES, sizeof(lightIndices) / sizeof(int), GL_UNSIGNED_INT, 0);


		// Tell OpenGL which Shader Program we want to use
		shaderProgram.Activate();
		camera.updateMatrix(45.0f, 0.1f, 100.00f);
		camera.Inputs(window, deltaTime);
		shaderProgram.SetUniformValueF("scale", 1.00f);

		// Bind the VAO so OpenGL knows to use it
		VAO1.Bind();
		texture.Bind();
		shaderProgram.SeUniFormValueMattrix("model", model);
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
	lightShader.Delete();

	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}