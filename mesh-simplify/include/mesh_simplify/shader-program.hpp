#pragma once

#include "mesh_simplify/base.hpp"

namespace aiza_dev::mesh_simplify {

class ShaderProgram {

public:
	ShaderProgram(const ShaderProgram&) = default;
	ShaderProgram(ShaderProgram&&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = default;
	ShaderProgram& operator=(ShaderProgram&&) = delete;
	ShaderProgram(const fpath& vertex_shader_path, const fpath& fragment_shader_path);
	~ShaderProgram();

	auto use() const -> void;

	auto set_uniform(const std::string& name, int value) const -> void;
	auto set_uniform(const std::string& name, float value) const -> void;
	auto set_uniform(const std::string& name, const vec3f& value) const -> void;
	auto set_uniform(const std::string& name, const mat4f& value) const -> void;

private:
	unsigned int _id;
};

} // namespace aiza_dev::mesh_simplify