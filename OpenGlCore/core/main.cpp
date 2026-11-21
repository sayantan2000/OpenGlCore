#include <iostream>
#include "Application.h"

// Entry point. All setup, the render loop and teardown live in Application; GL resources
// are owned by RAII types and freed before the GL context (Window) is destroyed.
//
// Usage: OpenGlCore [--project <path>]   (the Engine Hub passes the selected project path here)
int main(int argc, char** argv)
{
	try
	{
		std::string projectPath;
		for (int i = 1; i < argc; ++i)
		{
			std::string a = argv[i];
			if ((a == "--project" || a == "-p") && i + 1 < argc) projectPath = argv[++i];
			else if (a.rfind("--project=", 0) == 0) projectPath = a.substr(10);
		}
		ogl::Application app;
		app.SetProjectPath(projectPath);
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "[FATAL] " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
