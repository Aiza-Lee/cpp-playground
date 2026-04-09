#pragma once

#include "mesh_simplify/window.hpp"
#include "mesh_simplify/base.hpp"

namespace aiza_dev::mesh_simplify {

class App {
public:
	auto run(const fpath& model_path, const fpath& vertex_shader_path, const fpath& fragment_shader_path) -> void;

private:
	Window _window{800, 600, "Hello Mesh Simplify"};
};

} // namespace aiza_dev::mesh_simplify