#include <array>

#include <fmt/base.h>
#include <glad/glad.h>

#include "mesh_simplify/shader-program.hpp"
#include "mesh_simplify/base.hpp"
#include "mesh_simplify/utils/utils.hpp"

namespace aiza_dev::mesh_simplify {

namespace {

auto _compile_shader(u32 type, const std::string& source) -> u32 {
	u32 shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	int success = 0;
	std::array<char, 512> info_log{};
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, nullptr, info_log.data());
		fmt::print(stderr, "Shader compile error:\n{}\n", info_log.data());
	}

	return shader;
}

auto _get_location(u32 program_id, const std::string& name) -> int {
	int location = glGetUniformLocation(program_id, name.c_str());
	if (location == -1) {
		fmt::print(stderr, "Warning: uniform '{}' not found in shader\n", name);
	}
	return location;
}

} // namespace

ShaderProgram::ShaderProgram(const fpath& vertex_shader_path, const fpath& fragment_shader_path) {

	auto vertex_shader_source = utils::read_file(vertex_shader_path);
	auto fragment_shader_source = utils::read_file(fragment_shader_path);

	u32 vertex_shader = _compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
	u32 fragment_shader = _compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

	_id = glCreateProgram();
	glAttachShader(_id, vertex_shader);
	glAttachShader(_id, fragment_shader);
	glLinkProgram(_id);

	int success = 0;
	std::array<char, 512> info_log{};
	glGetProgramiv(_id, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(_id, 512, nullptr, info_log.data());
		fmt::print(stderr, "Shader program link error:\n{}\n", info_log.data());
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(_id);
}

auto ShaderProgram::use() const -> void {
	glUseProgram(_id);
}

auto ShaderProgram::set_uniform(const std::string& name, int value) const -> void {
	int location = _get_location(_id, name);
	glUniform1i(location, value);
}

auto ShaderProgram::set_uniform(const std::string& name, float value) const -> void {
	int location = _get_location(_id, name);
	glUniform1f(location, value);
}

auto ShaderProgram::set_uniform(const std::string& name, const vec3f& value) const -> void {
	int location = _get_location(_id, name);
	glUniform3f(location, value[0], value[1], value[2]);
}

auto ShaderProgram::set_uniform(const std::string& name, const mat4f& value) const -> void {
	int location = _get_location(_id, name);
	glUniformMatrix4fv(location, 1, GL_FALSE, value.data());
}


} // namespace aiza_dev::mesh_simplify