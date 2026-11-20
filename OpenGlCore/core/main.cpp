#include <iostream>
#include "Application.h"

// Entry point. All setup, the render loop and teardown live in Application; GL resources
// are owned by RAII types and freed before the GL context (Window) is destroyed.
int main()
{
	try
	{
		Application app;
		app.Run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "[FATAL] " << e.what() << std::endl;
		return -1;
	}
	return 0;
}
