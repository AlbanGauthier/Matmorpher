#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Primitive;
struct Material;
struct SceneState;

enum class Mode {
	PBR = 0,
	Interpolation = 1
};

class Shader {
private:
	// the program ID
	GLuint programID = 0;
	std::vector<const char*> path_to_shaders;

public:
	Shader() {};
	Shader(const Shader&) = delete;
	void operator=(const Shader&) = delete;
	~Shader() {
		if (programID != 0)
			glDeleteProgram(programID);
	};

	unsigned int getProgramID() { return programID; }

	void loadShaders(std::vector<const char*>);
	void loadShader(const char*);
	bool reloadShader();
	GLuint init(const char* path, unsigned int shaderType) const;

	// use/activate the shader
	void use() const;

	// utility uniform functions
	void setBool(const std::string& name, bool value) const
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
	}
	void setInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
	}
	void setFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
	}
	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		glUniform2fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string& name, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(programID, name.c_str()), x, y);
	}
	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(programID, name.c_str()), x, y, z);
	}
	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		glUniform4fv(glGetUniformLocation(programID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string& name, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(programID, name.c_str()), x, y, z, w);
	}
	void setMat2(const std::string& name, const glm::mat2& mat) const
	{
		glUniformMatrix2fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void setMat3(const std::string& name, const glm::mat3& mat) const
	{
		glUniformMatrix3fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
	}
	void setVec4Array(const std::string& name, const glm::vec4* values, int number_of_val) const {
		GLuint loc = glGetUniformLocation(programID, name.c_str());
		glProgramUniform4fv(programID, loc, number_of_val, glm::value_ptr(*values));
	}
};