#include <filesystem>
#include <fmt/core.h>

#include "mesh_simplify/app.hpp"

constexpr auto ASSETS_DIR = MESH_SIMPLIFY_ASSETS_DIR;
const auto MODEL_PATH 
	= std::filesystem::path(ASSETS_DIR) 
	/ "meshes" 
	/ "portable_cassette_player_4k.gltf";

const auto VERTEX_SHADER_PATH 
	= std::filesystem::path(ASSETS_DIR) 
	/ "shaders" 
	/ "line-frame.vert";

const auto FRAGMENT_SHADER_PATH 
	= std::filesystem::path(ASSETS_DIR) 
	/ "shaders" 
	/ "line-frame.frag";

int main() {

	aiza_dev::mesh_simplify::App app;

	try {
		app.run(MODEL_PATH, VERTEX_SHADER_PATH, FRAGMENT_SHADER_PATH);
	} catch (const std::exception& e) {
		fmt::print(stderr, "Error: {}\n", e.what());
	}

	return 0;
}