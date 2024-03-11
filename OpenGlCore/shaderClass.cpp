#include"shaderClass.h"

// Reads a text file and outputs a string with everything in the text file
std::string get_file_contents(const char* filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

// Constructor that build the Shader Program from 2 different shaders
Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
	// Read vertexFile and fragmentFile and store the strings
	std::string vertexCode = get_file_contents(vertexFile);
	std::string fragmentCode = get_file_contents(fragmentFile);

	// Convert the shader source strings into character arrays
	const char* vertexSource = vertexCode.c_str();
	const char* fragmentSource = fragmentCode.c_str();

	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);
	CompileError(vertexShader, "VERTEX",vertexFile);

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(fragmentShader);
	CompileError(fragmentShader, "FRAGMENT",fragmentFile);

	// Create Shader Program Object and get its reference
	ID = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(ID);
	CompileError(ID, "PROGRAM");

	// Delete the now useless Vertex and Fragment Shader objects
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}

// Activates the Shader Program
void Shader::Activate()
{
	glUseProgram(ID);
}

// Deletes the Shader Program
void Shader::Delete()
{
	glDeleteProgram(ID);
}

void Shader::CompileError(unsigned int shader, const char* type,const char* filename)
{

	GLint HasCompiled;
	char infolog[1024];
	if (type != "PROGRAM") {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &HasCompiled);
		if (HasCompiled == GL_FALSE) {
			glGetShaderInfoLog(shader, sizeof(infolog), nullptr, infolog);
			std::wcout << "SHADER_COMPILATION_ERR for :" << type << "\n"<< filename << std::endl;
			std::wcout << infolog;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_COMPILE_STATUS, &HasCompiled);
		if (HasCompiled == GL_FALSE) {
			glGetProgramInfoLog(shader, sizeof(infolog), nullptr, infolog);
			std::wcout << "SHADER_COMPILATION_ERR for :" << type << "\n" << filename << std::endl;
			std::wcout << infolog;
		}
	}
}

void Shader::CompileError(unsigned int shader, const char* type)
{
	GLint HasCompiled;
	char infolog[1024];
	if (type != "PROGRAM") {
		glGetShaderiv(shader, GL_COMPILE_STATUS, &HasCompiled);
		if (HasCompiled == GL_FALSE) {
			glGetShaderInfoLog(shader, sizeof(infolog), nullptr, infolog);
			std::wcout << "SHADER_COMPILATION_ERR for :" << type << "\n"  << std::endl;
			std::wcout << infolog;
		}
	}
	else
	{
		glGetProgramiv(shader, GL_COMPILE_STATUS, &HasCompiled);
		if (HasCompiled == GL_FALSE) {
			glGetProgramInfoLog(shader, sizeof(infolog), nullptr, infolog);
			std::wcout << "SHADER_COMPILATION_ERR for :" << type << "\n"  << std::endl;
			std::wcout << infolog;
		}
	}
}

void Shader::SetUniformValueF(const char* UniForm, float a_data)
{
	glUniform1f(GetUniFormId(UniForm), a_data);
}

void Shader::SeUniFormValueMattrix(const char* Uniform, glm::mat4& mat)
{
	glUniformMatrix4fv(GetUniFormId(Uniform), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::SetUniformValueVec4(const char* Uniform, glm::vec4 vector)
{
	glUniform4f(GetUniFormId(Uniform), vector.x, vector.y, vector.z, vector.w);
}

void Shader::SetUniformValueVec3(const char* Uniform, glm::vec3 vector)
{
	glUniform3f(GetUniFormId(Uniform), vector.x, vector.y, vector.z);
}

GLuint Shader::GetUniFormId(const char* UniForm)
{
	return glGetUniformLocation(ID, UniForm);
}
