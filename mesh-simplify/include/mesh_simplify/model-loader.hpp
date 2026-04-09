#pragma once

#include <filesystem>

#include <tiny_gltf.h>

namespace aiza_dev::mesh_simplify {

class ModelLoader {
public:
	static auto load_model(const std::filesystem::path& path) -> tinygltf::Model;
};

} // namespace aiza_dev::mesh_simplify
