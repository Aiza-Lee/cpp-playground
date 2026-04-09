#pragma once

#include <cstdint>
#include <filesystem>

#include <Eigen/Core>

namespace aiza_dev::mesh_simplify {
	
using i8 = int8_t;
using u8 = uint8_t;

using i16 = int16_t;
using u16 = uint16_t;

using i32 = int32_t;
using u32 = uint32_t;

using i64 = int64_t;
using u64 = uint64_t;

using byte = uint8_t;

using f32 = float;
using f64 = double;
using f128 = long double;

using vec2f = Eigen::Vector2f;
using vec3f = Eigen::Vector3f;
using vec4f = Eigen::Vector4f;

using mat2f = Eigen::Matrix2f;
using mat3f = Eigen::Matrix3f;
using mat4f = Eigen::Matrix4f;

using vec2i = Eigen::Vector2i;
using vec3i = Eigen::Vector3i;
using vec4i = Eigen::Vector4i;

using fpath = std::filesystem::path;

} // namespace aiza_dev::mesh_simplify