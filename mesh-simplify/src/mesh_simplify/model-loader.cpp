#include <stdexcept>
#include <string>

#include <fmt/core.h>

#include "mesh_simplify/model-loader.hpp"

namespace aiza_dev::mesh_simplify {

auto ModelLoader::load_model(const std::filesystem::path& path) -> tinygltf::Model {
	auto model = tinygltf::Model{};
	auto loader = tinygltf::TinyGLTF{};
	std::string err;
	std::string warn;

	bool ok = false;
	if (path.extension() == ".glb") {
		ok = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
	} else {
		ok = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
	}

	if (!warn.empty()) {
		fmt::print("Warn: {}\n", warn);
	}
	if (!err.empty()) {
		fmt::print("Err: {}\n", err);
	}
	if (!ok) {
		throw std::runtime_error("Failed to load model");
	}

	return model;
}

} // namespace aiza_dev::mesh_simplify
