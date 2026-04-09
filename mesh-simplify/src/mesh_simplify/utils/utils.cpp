#include <fstream>

#include <fmt/base.h>

#include "mesh_simplify/utils/utils.hpp"

namespace aiza_dev::mesh_simplify::utils {

auto read_file(const fpath& path) -> std::string {
	std::ifstream file(path);
	if (!file.is_open()) {
		fmt::print(stderr, "Failed to open file: {}\n", path.string());
		return {};
	}
	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

} // namespace aiza_dev::mesh_simplify::utils