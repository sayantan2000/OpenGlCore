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
#include"Primitive.h";
#include "Time.h";

constexpr float const FOV = 60.00f;

float oldTimeSinceStart = 0;
int HEIGHT = 1080, WIDTH = 1920;

int main()
{
	Primitive p(PrimitiveType::pyramid);
	Primitive sun(PrimitiveType::sphere);

	//PrimitIve p = PrimitIve::GetPrimiTive(PrimitIve::Sphere);
	HEIGHT = GetSystemMetrics(SM_CYSCREEN);
	WIDTH = GetSystemMetrics(SM_CXSCREEN);
	float currentFrame = glfwGetTime();
	float lastFrame = currentFrame;

	float a = 0;
	float speed = 0.6;

	a += Time::_DeltaTime * speed;
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

	// Scope block: every RAII GL wrapper below is destroyed at this block's closing
	// brace, which runs BEFORE glfwDestroyWindow/glfwTerminate. Freeing GL objects while
	// the context is still current is required for the deletes to actually take effect.
	{

	// Generates Shader object using shaders defualt.vert and default.frag
	Shader shaderProgram("./Shader/default.vert", "./SHader/default.frag");



	// Generates Vertex Array Object and binds it
	VerTexArray VAO1;
	VAO1.Bind();

	// Generates Vertex Buffer Object and links it to vertices
	VerTexBuffer VBO1(p.vertices.data(), p.VerticesStride());
	// Generates Element Buffer Object and links it to indices
	Triangle EBO1(p.Indices.data(), p.TriangleStride());

	// Links VBO to VAO
	VAO1.LinkAttriBute(VBO1, 0, 3, GL_FLOAT, 11 * sizeof(float), (void*)0);
	VAO1.LinkAttriBute(VBO1, 1, 3, GL_FLOAT, 11 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttriBute(VBO1, 2, 2, GL_FLOAT, 11 * sizeof(float), (void*)(6 * sizeof(float)));
	VAO1.LinkAttriBute(VBO1, 3, 3, GL_FLOAT, 11 * sizeof(float), (void*)(8 * sizeof(float)));
	// Unbind all to prevent accidentally modifying them
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	Shader lightShader("./Shader/Light.vert", "./Shader/Light.frag");
	// Generates Vertex Array Object and binds it
	VerTexArray lightVAO;
	lightVAO.Bind();
	// Generates Vertex Buffer Object and links it to vertices
	VerTexBuffer lightVBO(sun.vertices.data(), sun.VerticesStride());
	// Generates Element Buffer Object and links it to indices
	Triangle lightEBO(sun.Indices.data(), sun.TriangleStride());
	// Links VBO attributes such as coordinates and colors to VAO
	lightVAO.LinkAttriBute(lightVBO, 0, 3, GL_FLOAT, 3 * sizeof(float), (void*)0);
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

	glm::vec4 LightColor{ 1.00f,0.81f,0.50f,1 };

	//getting the id of the uniform variable from active shader
	shaderProgram.Activate();
	shaderProgram.SetUniformValueVec3("lightPos", lightPos);
	shaderProgram.SetUniformValueVec4("lightcolor", LightColor);
	lightShader.Activate();
	lightShader.SeUniFormValueMattrix("model", lightModel);
	lightShader.SetUniformValueVec4("LightColor", LightColor);
	lightShader.SetUniformValueVec3("Scale", glm::vec3(0.2f));




	Camera camera(WIDTH, HEIGHT, glm::vec3(0.0f, .5f, 2.0f));

	float rotation = 0.0f;
	double prevTime = glfwGetTime();
	glEnable(GL_DEPTH_TEST);
	glm::mat4 model = glm::mat4(1.00f);

	double _prevTime = 0.0;
	double crntTime = 0.0;
	double timeDiff;
	// Keeps track of the amount of frames in timeDiff
	unsigned int counter = 0;
	// Main while loop
	while (!glfwWindowShouldClose(window))
	{
		crntTime = glfwGetTime();
		timeDiff = crntTime - prevTime;
		counter++;

		if (timeDiff >= 1.0 / 1000)
		{
			// Creates new title
			std::string FPS = std::to_string((1.0 / timeDiff) * counter);
			std::string ms = std::to_string((timeDiff / counter) * 1000);
			std::string newTitle = "OpenGlCore - " + FPS + "FPS / " + ms + "ms";
			glfwSetWindowTitle(window, newTitle.c_str());

			// Resets times and counter
			_prevTime = crntTime;
			counter = 0;

			// Use this if you have disabled VSync
			//camera.Inputs(window);
		}
		currentFrame = glfwGetTime();
		Time::_DeltaTime = currentFrame - lastFrame;

		lastFrame = currentFrame;

		// Smooth, frame-rate-independent spin. `rotation` is an accumulating angle in
		// degrees advanced by (speed * dt); the model matrix is REBUILT from identity
		// each frame. Previously the matrix was multiplied onto itself every frame and
		// the angle was not time-scaled, so the pyramid spun uncontrollably at high FPS.
		const float rotationSpeedDegPerSec = 20.0f;
		rotation += rotationSpeedDegPerSec * Time::_DeltaTime;
		if (rotation >= 360.0f) rotation -= 360.0f;
		model = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
		shaderProgram.SetUniformValueVec3("CamPos", camera.Position);
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
		glDrawElements(GL_TRIANGLES, sun.Indices.size(), GL_UNSIGNED_INT, 0);


		// Tell OpenGL which Shader Program we want to use
		shaderProgram.Activate();
		camera.updateMatrix(FOV, 0.3f, 100);
		camera.Inputs(window, Time::_DeltaTime);
		shaderProgram.SetUniformValueF("scale", 1.00f);
		// Upload the camera matrix BEFORE drawing so this frame's draw uses the current
		// view/projection (previously uploaded after glDrawElements -> one frame stale).
		camera.Matrix(shaderProgram, "camMatrix");

		// Bind the VAO so OpenGL knows to use it
		VAO1.Bind();
		texture.Bind();
		shaderProgram.SeUniFormValueMattrix("model", model);
		// Draw primitives, number of indices, datatype of indices, index of indices
		glDrawElements(GL_TRIANGLES, p.Indices.size(), GL_UNSIGNED_INT, 0);


		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}

	} // end GL-object scope: all RAII wrappers (shaders, VAOs, VBOs, EBOs, texture)
	  // are destroyed here, while the GL context is still current.

	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}