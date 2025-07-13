#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "ShaderProgram.hpp"


ShaderProgram::ShaderProgram(const std::filesystem::path& VS_file, const std::filesystem::path& FS_file) {
	std::vector<GLuint> shader_ids;

	// compile shaders and store IDs for linker
	shader_ids.push_back(compile_shader(VS_file, GL_VERTEX_SHADER));
	shader_ids.push_back(compile_shader(FS_file, GL_FRAGMENT_SHADER));

	// link all compiled shaders into shader_program 
	ID = link_shader(shader_ids);
}

GLint ShaderProgram::getUniformLocation(const std::string& name) {

	auto it = uniformCache.find(name);
	if (it != uniformCache.end()) return it->second;

	GLint location = glGetUniformLocation(ID, name.c_str());
	if (location == -1) {
		std::cerr << "Warning: uniform '" << name << "' not found in shader.\n";
		return location;
	}
	uniformCache[name] = location;
	return location;

}

// set uniform to float
void ShaderProgram::setUniform(const std::string& name, const float val) {
	auto loc = getUniformLocation(name); // get location of uniform
	if (loc == -1) {
		return;
	}
	glProgramUniform1f(ID, loc, val);  // set uniform
}

// set uniform to int
void ShaderProgram::setUniform(const std::string& name, const int val) {
	auto loc = getUniformLocation(name); // get location of uniform
	if (loc == -1) {
		return;
	}
	glProgramUniform1i(ID, loc, val); // set uniform
}

// set uniform to vec3
void ShaderProgram::setUniform(const std::string& name, const glm::vec3& val) {
	auto loc = getUniformLocation(name); // get location of uniform
	if (loc == -1) {
		return;
	}
	glProgramUniform3fv(ID, loc, 1, glm::value_ptr(val)); // set uniform
}

// set uniform to vec4
void ShaderProgram::setUniform(const std::string& name, const glm::vec4& in_vec4) {
	auto loc = getUniformLocation(name);  // get location of uniform
	if (loc == -1) {
		return;
	}
	glProgramUniform4fv(ID, loc, 1, glm::value_ptr(in_vec4));  // set uniform
}

// set uniform to mat3
void ShaderProgram::setUniform(const std::string& name, const glm::mat3& val) {
	auto loc = getUniformLocation(name);  // get location of uniform
	if (loc == -1) {
		return;
	}
	glProgramUniformMatrix3fv(ID, loc, 1, GL_FALSE, glm::value_ptr(val));  // set uniform
}

// set uniform to mat4
void ShaderProgram::setUniform(const std::string& name, const glm::mat4& val) {
	auto loc = getUniformLocation(name);  // get location of uniform
	if (loc == -1) {
		return;
	}
	glProgramUniformMatrix4fv(ID, loc, 1, GL_FALSE, glm::value_ptr(val));  // set uniform
}

// Get shader compilator errors
std::string ShaderProgram::getShaderInfoLog(const GLuint obj) {
	int infoLogLength;
	std::string infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infoLogLength);  // get length of info log

	if (infoLogLength > 0) {
		std::vector<char> vec(infoLogLength); // create vector of chars with size of info log
		glGetShaderInfoLog(obj, infoLogLength, NULL, vec.data()); // get info log
		infoLog.assign(begin(vec), end(vec)); // assign info log to string
	}
	return infoLog;
}

// Get shader linker errors
std::string ShaderProgram::getProgramInfoLog(const GLuint obj) {
	int infoLogLength = 0;
	std::string infoLog;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infoLogLength);  // get length of info log

	if (infoLogLength > 0) {
		std::vector<char> vec(infoLogLength); // create vector of chars with size of info log
		glGetProgramInfoLog(obj, infoLogLength, NULL, vec.data()); // get info log
		infoLog.assign(begin(vec), end(vec)); // assign info log to string
	}
	return infoLog;
}

// Compile shader
GLuint ShaderProgram::compile_shader(const std::filesystem::path& source_file, const GLenum type) {
	GLuint shader_h;

	shader_h = glCreateShader(type);  // create shader object of type (GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc
	std::string shader_source = textFileRead(source_file);

	// check if shader source is empty (file not found)
	if (shader_source.empty()) {
		throw std::runtime_error("Failed to read shader file: " + source_file.string());
	}

	GLchar const* shader_source_c = shader_source.c_str();  // convert string to const char
	glShaderSource(shader_h, 1, &shader_source_c, NULL);  // set shader source

	glCompileShader(shader_h);
	{
		// check compile result , print info & throw error (if any)
		GLint cmpl_status;
		glGetShaderiv(shader_h, GL_COMPILE_STATUS, &cmpl_status);
		// check compile status
		if (cmpl_status == GL_FALSE) {
			std::cerr << getShaderInfoLog(shader_h);
			glDeleteShader(shader_h);  // delete shader if compile failed
			throw std::runtime_error("Shader compile err.\n");
		}
	}

	return shader_h;
}

GLuint ShaderProgram::link_shader(const std::vector<GLuint> shader_ids) {
	GLuint prog_h = glCreateProgram();

	// link all shaders to program
	for (auto const id : shader_ids)
		glAttachShader(prog_h, id);

	// link program => start compilation
	glLinkProgram(prog_h);
	{
		GLint link_status;
		glGetProgramiv(prog_h, GL_LINK_STATUS, &link_status);
		if (link_status == GL_FALSE) {
			std::cerr << getProgramInfoLog(prog_h);
			glDeleteProgram(prog_h);  // delete program if link failed
			throw std::runtime_error("Shader link err.\n");
		}
	}

	// free shaders after linking
	for (auto const id : shader_ids) {
		glDetachShader(prog_h, id);  // detach shaders from program
		glDeleteShader(id);  // delete shaders
	}

	return prog_h;
}

std::string ShaderProgram::textFileRead(const std::filesystem::path& filename) {
	std::ifstream file(filename);
	if (!file.is_open())
		throw std::runtime_error(std::string("Error opening file: ") + filename.string());
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}