#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include<glad/glad.h>
#include<string>
#include<fstream>
#include<sstream>
#include<iostream>
#include<cerrno>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>

std::string get_file_contents(const char* filename);

class Shader
{
public:
	// Reference ID of the Shader Program
	GLuint ID;
	// Constructor that build the Shader Program from 2 different shaders
	Shader(const char* vertexFile, const char* fragmentFile);

	// Activates the Shader Program
	void Activate();
	// Deletes the Shader Program
	void Delete();

	void CompileError(unsigned int shader, const char* type,const char* filename);
	void CompileError(unsigned int shader, const char* type);
	void SetUniformValueF(const char* UniForm, float a_data);
	void SeUniFormValueMattrix(const char* Uniform, glm::mat4& mat);
	void SetUniformValueVec4(const char* Uniform, glm::vec4 vector);
	void SetUniformValueVec3(const char* Uniform, glm::vec3 vector);

private:
	GLuint GetUniFormId(const char* UniForm);
};
#endif