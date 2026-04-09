#pragma once

#include <string>

#include "mesh_simplify/base.hpp"

namespace aiza_dev::mesh_simplify::utils {

auto read_file(const fpath& path) -> std::string;

} // namespace aiza_dev::mesh_simplify::utils