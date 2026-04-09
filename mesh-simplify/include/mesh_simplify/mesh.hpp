#pragma once

#include <vector>

#include "mesh_simplify/base.hpp"
#include "mesh_simplify/model-loader.hpp"
#include <Eigen/Core>
#include <tiny_gltf.h>

namespace aiza_dev::mesh_simplify {

class Mesh {
public:
	Mesh() = default;
	Mesh(const tinygltf::Model& model);
	Mesh( const std::vector<vec3f>& vertices, const std::vector<vec3i>& faces);

	[[nodiscard]] auto vertices() const -> const std::vector<vec3f>& { return _vertices; }
	[[nodiscard]] auto faces() const -> const std::vector<vec3i>& { return _faces; }
	[[nodiscard]] auto edges() const -> const std::vector<vec2i>& {
		if (!_valid_edges) {
			_process_edges();
			_valid_edges = true;
		}
		return _edges;
	}
	[[nodiscard]] auto bounds() const -> std::pair<vec3f, vec3f> {
		if (!_valid_bounds) {
			_compute_bounds();
			_valid_bounds = true;
		}
		return {_min_bound, _max_bound};
	}

	[[nodiscard]] auto vertex_degrees() const -> const std::vector<int>& {
		if (!_valid_vertex_degrees) {
			_compute_vertex_degrees();
			_valid_vertex_degrees = true;
		}
		return degree;
	}

private:
	std::vector<vec3f> _vertices; // 模型定点
	std::vector<vec3i> _faces;    // 模型面
	mutable std::vector<vec2i> _edges;    // 模型边

	mutable vec3f _min_bound{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
	mutable vec3f _max_bound{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

	mutable std::vector<int> degree;

	mutable bool _valid_edges = false;
	mutable bool _valid_bounds = false;
	mutable bool _valid_vertex_degrees = false;

	auto _process_edges() const -> void;
	auto _compute_bounds() const -> void;
	auto _compute_vertex_degrees() const -> void;
};

} // namespace aiza_dev::mesh_simplify