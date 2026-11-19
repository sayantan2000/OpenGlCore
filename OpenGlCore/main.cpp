#include<iostream>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include"shaderClass.h"
#include"VerTexArray.h"
#include"VerTexBuffer.h"
#include"ElementBuffer.h"
#include"Mesh.h"
#include"Text.h"
#include"Model.h"
#include "Texture.h"
#include "Camera.h"
#include"Primitive.h"
#include "Time.h"

constexpr float const FOV = 60.00f;
// Set to false for a windowed 1280x720; true uses the primary monitor's full resolution.
constexpr bool FULLSCREEN = true;

float oldTimeSinceStart = 0;
int HEIGHT = 720, WIDTH = 1280;

int main()
{
	Primitive p(PrimitiveType::pyramid);
	Primitive sun(PrimitiveType::sphere);

	//PrimitIve p = PrimitIve::GetPrimiTive(PrimitIve::Sphere);
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

	// Determine window size + target monitor via GLFW (no Win32 dependency).
	// Fullscreen: use the primary monitor's current video mode resolution.
	GLFWmonitor* targetMonitor = nullptr;
	if (FULLSCREEN)
	{
		GLFWmonitor* primary = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primary);
		if (mode != nullptr)
		{
			WIDTH = mode->width;
			HEIGHT = mode->height;
		}
		targetMonitor = primary; // passing a monitor makes the window fullscreen
	}

	// Create the GLFW window (fullscreen on the primary monitor, or windowed).
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGLCore", targetMonitor, nullptr);
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
	// Wrapped in try/catch so a missing shader/texture exits cleanly (RAII still unwinds)
	// instead of std::terminate -> abort.
	try {

	// Generates Shader object using shaders defualt.vert and default.frag
	Shader shaderProgram("./Shader/default.vert", "./SHader/default.frag");



	// Pyramid mesh: layout is pos(3) color(3) uv(2) normal(3) = 11 floats/vertex.
	Mesh pyramidMesh(p.vertices, p.Indices,
		{ {3, GL_FLOAT}, {3, GL_FLOAT}, {2, GL_FLOAT}, {3, GL_FLOAT} });

	Shader lightShader("./Shader/Light.vert", "./Shader/Light.frag");
	// Light-source sphere mesh: position-only (3 floats/vertex).
	Mesh sphereMesh(sun.vertices, sun.Indices, { {3, GL_FLOAT} });

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
	lightShader.SetUniformMat4("model", lightModel);
	lightShader.SetUniformValueVec4("LightColor", LightColor);
	lightShader.SetUniformValueVec3("Scale", glm::vec3(0.2f));

	// Shader for loaded models: lit, solid albedo base color (no texture).
	Shader modelShader("./Shader/default.vert", "./Shader/albedo.frag");
	modelShader.Activate();
	modelShader.SetUniformValueVec3("lightPos", lightPos);
	modelShader.SetUniformValueVec4("lightcolor", LightColor);
	modelShader.SetUniformValueF("scale", 1.0f);
	glm::vec3 cubeAlbedo(0.15f, 0.55f, 0.85f); // base color (teal-blue)
	modelShader.SetUniformValueVec3("albedo", cubeAlbedo);




	Camera camera(WIDTH, HEIGHT, glm::vec3(0.0f, .5f, 2.0f));

	// On-screen text overlay (FreeType). Font is loaded from ./Fonts/arial.ttf.
	Text hud("./Fonts/arial.ttf", 48, WIDTH, HEIGHT);
	std::string hudText = "FPS: --";

	// Load an FBX model via Assimp. Compute a uniform scale that fits it to ~2 units
	// (meters) regardless of the file's native units, and its center for positioning.
	Model cubeModel("./Models/Cube.fbx");
	float cubeFitScale = cubeModel.ScaleToFit(2.0f);
	glm::vec3 cubeCenter = cubeModel.Center();

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
		// Per-frame delta time (used for frame-rate-independent motion).
		Time::_DeltaTime = (float)(crntTime - lastFrame);
		lastFrame = crntTime;
		counter++;

		// Update the FPS counter (title + HUD) ~twice per second over real elapsed time.
		timeDiff = crntTime - prevTime;
		if (timeDiff >= 0.5)
		{
			int fpsInt = (int)(counter / timeDiff);
			double ms = (timeDiff / counter) * 1000.0;
			std::string newTitle = "OpenGlCore - " + std::to_string(fpsInt) + " FPS / "
				+ std::to_string(ms) + " ms";
			glfwSetWindowTitle(window, newTitle.c_str());
			hudText = "FPS: " + std::to_string(fpsInt);

			prevTime = crntTime;
			counter = 0;
		}

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
		// Export the camMatrix to the Vertex Shader of the light source
		camera.Matrix(lightShader, "camMatrix");
		lightShader.SetUniformMat4("model", lightModel);
		lightShader.SetUniformValueF("_Time", (float)glfwGetTime());
		// Mesh::Draw binds its own VAO and issues the indexed draw.
		sphereMesh.Draw();


		// Tell OpenGL which Shader Program we want to use
		shaderProgram.Activate();
		camera.updateMatrix(FOV, 0.3f, 100);
		camera.Inputs(window, Time::_DeltaTime);
		shaderProgram.SetUniformValueF("scale", 1.00f);
		// Upload the camera matrix BEFORE drawing so this frame's draw uses the current
		// view/projection (previously uploaded after glDrawElements -> one frame stale).
		camera.Matrix(shaderProgram, "camMatrix");

		texture.Bind();
		shaderProgram.SetUniformMat4("model", model);
		// Mesh::Draw binds its own VAO and issues the indexed draw.
		pyramidMesh.Draw();

		// Draw the Assimp-loaded model with the albedo (solid base color) shader.
		// Scale to ~2 units and recenter on its own bounds, placed to the side.
		modelShader.Activate();
		modelShader.SetUniformValueVec3("CamPos", camera.Position);
		camera.Matrix(modelShader, "camMatrix");
		glm::mat4 cubeModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 1.0f, 0.0f));
		cubeModelMat = glm::scale(cubeModelMat, glm::vec3(cubeFitScale));
		cubeModelMat = glm::translate(cubeModelMat, -cubeCenter);
		modelShader.SetUniformMat4("model", cubeModelMat);
		// Draw sets the albedo uniform per sub-mesh from each imported material color.
		cubeModel.Draw(modelShader);

		// Draw the on-screen HUD last (2D overlay, blended, depth-test disabled inside).
		hud.RenderText(hudText, 25.0f, (float)HEIGHT - 60.0f, 0.6f, glm::vec3(0.9f, 0.9f, 0.2f));


		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}

	} // end GL-object scope: all RAII wrappers (shaders, VAOs, VBOs, EBOs, texture)
	  // are destroyed here, while the GL context is still current.
	catch (const std::exception& e)
	{
		// RAII wrappers constructed before the throw are destroyed during stack
		// unwinding (context still current), so GL resources are freed correctly.
		std::cerr << "[FATAL] " << e.what() << std::endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}