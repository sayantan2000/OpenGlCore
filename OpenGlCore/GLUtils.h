#ifndef GLUTILS_H
#define GLUTILS_H

#include <glad/glad.h>
#include <iostream>

// Lightweight OpenGL error checking for a 3.3 context (no KHR_debug dependency).
// Drains the GL error queue and logs any errors with the originating file/line.
// Usage: call glCheckError() after a group of GL calls you want to validate.
inline GLenum glCheckError_(const char* file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		const char* error = "UNKNOWN";
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "GL_INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "GL_INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "GL_INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "GL_OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cerr << "[GL ERROR] " << error << " at " << file << ":" << line << std::endl;
	}
	return errorCode;
}

#define glCheckError() glCheckError_(__FILE__, __LINE__)

#endif
