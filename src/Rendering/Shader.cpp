#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <Rendering/SceneState.h>

class Camera;

void Shader::loadShaders(std::vector<const char*> paths)
{
	path_to_shaders = paths;

	if (path_to_shaders.size() != 5)
		assert(false);

	const char* vertexPath = path_to_shaders[0];
	const char* fragmentPath = path_to_shaders[1];
	const char* tescPath = path_to_shaders[2];
	const char* tesePath = path_to_shaders[3];
	const char* geometryPath = path_to_shaders[4];

	unsigned int vertex, fragment, tesc, tese, geometry;
	if (vertexPath != NULL)
		vertex = init(vertexPath, GL_VERTEX_SHADER);
	if (fragmentPath != NULL)
		fragment = init(fragmentPath, GL_FRAGMENT_SHADER);
	if (tescPath != NULL)
		tesc = init(tescPath, GL_TESS_CONTROL_SHADER);
	if (tesePath != NULL)
		tese = init(tesePath, GL_TESS_EVALUATION_SHADER);
	if (geometryPath != NULL)
		geometry = init(geometryPath, GL_GEOMETRY_SHADER);

	int success;
	char infoLog[512];

	// shader Program
	programID = glCreateProgram();

	if (vertexPath != NULL)
		glAttachShader(programID, vertex);
	if (fragmentPath != NULL)
		glAttachShader(programID, fragment);
	if (tescPath != NULL)
		glAttachShader(programID, tesc);
	if (tesePath != NULL)
		glAttachShader(programID, tese);
	if (geometryPath != NULL)
		glAttachShader(programID, geometry);

	glLinkProgram(programID);
	// print linking errors if any
	glGetProgramiv(programID, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(programID, 512, NULL, infoLog);
		std::string path = vertexPath;
		std::size_t found = path.rfind("/");
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << std::endl
			<< infoLog
			<< "in " + path.substr(found) + " shader"
			<< std::endl;
	}

	// delete the shaders as they're linked into our program now and no longer necessery
	if (vertexPath != NULL)
		glDeleteShader(vertex);
	if (fragmentPath != NULL)
		glDeleteShader(fragment);
	if (tescPath != NULL)
		glDeleteShader(tesc);
	if (tesePath != NULL)
		glDeleteShader(tese);
	if (geometryPath != NULL)
		glDeleteShader(geometry);
}

void Shader::loadShader(const char* path)
{
	GLuint shaderID;
	if (path != NULL)
		shaderID = init(path, GL_COMPUTE_SHADER);

	int success;
	char infoLog[512];

	// shader Program
	programID = glCreateProgram();

	if (path != NULL)
		glAttachShader(programID, shaderID);

	glLinkProgram(programID);
	// print linking errors if any
	glGetProgramiv(programID, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(programID, 1024, NULL, infoLog);
		std::string path = path;
		std::size_t found = path.rfind("/");
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED" << std::endl
			<< infoLog
			<< "in " + path.substr(found) + " shader"
			<< std::endl;
	}

	if (path != NULL)
		glDeleteShader(shaderID);
}

GLuint Shader::init(const char* path, unsigned int shaderType) const
{
	// 1. retrieve the shader source code from filePath
	std::string sourceCode;
	std::ifstream shaderFile;
	// ensure ifstream objects can throw exceptions:
	shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		// open files
		shaderFile.open(path);
		std::stringstream shaderStream;
		// read file's buffer contents into streams
		shaderStream << shaderFile.rdbuf();
		// close file handlers
		shaderFile.close();
		// convert stream into string
		sourceCode = shaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		std::cerr << path << std::endl;
	}
	const char* shaderCode = sourceCode.c_str();

	// 2. compile shaders
	GLuint shaderID;
	int success;
	char infoLog[512];

	// vertex Shader
	shaderID = glCreateShader(shaderType);
	// attach the shader source code to the shader object
	glShaderSource(shaderID, 1, &shaderCode, NULL);
	// compile the shader
	glCompileShader(shaderID);
	// print compile errors if any
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
		switch (shaderType)
		{
		case GL_VERTEX_SHADER:
			std::cout << "ERROR::SHADER::VERTEX_SHADER::COMPILATION_FAILED\n"
				<< infoLog << std::endl;
			break;
		case GL_FRAGMENT_SHADER:
			std::cout << "ERROR::SHADER::FRAGMENT_SHADER::COMPILATION_FAILED\n"
				<< infoLog << std::endl;
			break;
		case GL_TESS_CONTROL_SHADER:
			std::cout << "ERROR::SHADER::CONTROL_SHADER::COMPILATION_FAILED\n"
				<< infoLog << std::endl;
			break;
		case GL_TESS_EVALUATION_SHADER:
			std::cout << "ERROR::SHADER::EVALUATION_SHADER::COMPILATION_FAILED\n"
				<< infoLog << std::endl;
			break;
		case GL_GEOMETRY_SHADER:
			std::cout << "ERROR::SHADER::GL_GEOMETRY_SHADER::COMPILATION_FAILED\n"
				<< infoLog << std::endl;
			break;
		case GL_COMPUTE_SHADER:
			std::cout << "ERROR::SHADER::GL_COMPUTE_SHADER::COMPILATION_FAILED\n"
				<< infoLog << std::endl;
			break;
		default:
			std::cout << "wrong shader type in the debug log" << std::endl;
			break;
		}
	};

	return shaderID;
}

void Shader::use() const
{
	glUseProgram(programID);
}

bool Shader::reloadShader()
{
	const char* vertexPath = path_to_shaders[0];
	const char* fragmentPath = path_to_shaders[1];
	const char* tescPath = path_to_shaders[2];
	const char* tesePath = path_to_shaders[3];
	const char* geometryPath = path_to_shaders[4];

	unsigned int vertex, fragment, tesc, tese, geometry;
	if (vertexPath != NULL)
		vertex = init(vertexPath, GL_VERTEX_SHADER);
	if (fragmentPath != NULL)
		fragment = init(fragmentPath, GL_FRAGMENT_SHADER);
	if (tescPath != NULL)
		tesc = init(tescPath, GL_TESS_CONTROL_SHADER);
	if (tesePath != NULL)
		tese = init(tesePath, GL_TESS_EVALUATION_SHADER);
	if (geometryPath != NULL)
		geometry = init(geometryPath, GL_GEOMETRY_SHADER);

	int success;
	char infoLog[512];

	if (programID != 0)
		glDeleteProgram(programID);

	programID = glCreateProgram();

	if (vertexPath != NULL)
		glAttachShader(programID, vertex);
	if (fragmentPath != NULL)
		glAttachShader(programID, fragment);
	if (tescPath != NULL)
		glAttachShader(programID, tesc);
	if (tesePath != NULL)
		glAttachShader(programID, tese);
	if (geometryPath != NULL)
		glAttachShader(programID, geometry);

	glLinkProgram(programID);
	// print linking errors if any
	glGetProgramiv(programID, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(programID, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
			<< infoLog << std::endl;
	}

	// delete the shaders as they're linked into our program now and no longer necessery
	if (vertexPath != NULL)
		glDeleteShader(vertex);
	if (fragmentPath != NULL)
		glDeleteShader(fragment);
	if (tescPath != NULL)
		glDeleteShader(tesc);
	if (tesePath != NULL)
		glDeleteShader(tese);
	if (geometryPath != NULL)
		glDeleteShader(geometry);

	return success;
}