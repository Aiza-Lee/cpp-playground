#include "mesh_simplify/mesh.hpp"
#include "mesh_simplify/base.hpp"
#include "mesh_simplify/model-loader.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace aiza_dev::mesh_simplify {

namespace {

auto _extract_vertices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<vec3f>& out_vertices) -> void {
	auto it = primitive.attributes.find("POSITION");
	if (it == primitive.attributes.end()) return;

	const auto& accessor = model.accessors[static_cast<size_t>(it->second)];
	const auto& buffer_view = model.bufferViews[static_cast<size_t>(accessor.bufferView)];
	const auto& buffer = model.buffers[static_cast<size_t>(buffer_view.buffer)];

	const std::span<const byte> raw_bytes(buffer.data.data(), buffer.data.size());
	const size_t byte_offset = buffer_view.byteOffset + accessor.byteOffset;
	
	// POSITION 规定组件为 FLOAT，且每个元素为 VEC3 (所以总是 12 字节)
	const size_t byte_stride = buffer_view.byteStride ? buffer_view.byteStride : sizeof(float) * 3;

	for (size_t i = 0; i < accessor.count; ++i) {
		const size_t ele_offset = byte_offset + (i * byte_stride);
		
		// 直接通过偏移取点（假设这里完全符合 FLOAT VEC3 规范）
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		const auto* pos_ptr = reinterpret_cast<const float*>(raw_bytes.data() + ele_offset);
		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		out_vertices.emplace_back(pos_ptr[0], pos_ptr[1], pos_ptr[2]);
	}
}

auto _extract_faces(const tinygltf::Model& model, const tinygltf::Primitive& primitive, std::vector<vec3i>& out_faces) -> void {
	if (primitive.indices < 0) return;

	const auto& accessor = model.accessors[static_cast<size_t>(primitive.indices)];
	const auto& buffer_view = model.bufferViews[static_cast<size_t>(accessor.bufferView)];
	const auto& buffer = model.buffers[static_cast<size_t>(buffer_view.buffer)];

	const std::span<const byte> raw_bytes(buffer.data.data(), buffer.data.size());
	const size_t byte_offset = buffer_view.byteOffset + accessor.byteOffset;
	
	// 计算索引的真实单元素跨步
	size_t component_size = 0;
	switch (accessor.componentType) {
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  component_size = 1; break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: component_size = 2; break;
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   component_size = 4; break;
		default: return; // 暂不处理非标准情况
	}
	const size_t byte_stride = buffer_view.byteStride ? buffer_view.byteStride : component_size;

	// accessor.count 是单个顶点的索引总数，我们需要每 3 个组成一个面
	for (size_t i = 0; i < accessor.count; i += 3) {
		std::array<u32, 3> face_indices{};
		
		for (size_t j = 0; j < 3; ++j) {
			if (i + j >= accessor.count) break;
			
			const size_t ele_offset = byte_offset + ((i + j) * byte_stride);
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			const byte* ptr = raw_bytes.data() + ele_offset;
			
			// 根据真实数据类型转型并提取值
			switch (accessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
					face_indices.at(j) = *reinterpret_cast<const u8*>(ptr);
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
					face_indices.at(j) = *reinterpret_cast<const u16*>(ptr);
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
					// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
					face_indices.at(j) = *reinterpret_cast<const u32*>(ptr);
					break;
				default:
					face_indices.at(j) = 0; // 不支持的类型，默认为0
					break;
			}
		}
		
		out_faces.emplace_back(face_indices[0], face_indices[1], face_indices[2]);
	}
}

} // namespace

Mesh::Mesh(const tinygltf::Model& model) {
	_vertices.reserve(model.meshes.size() * 1000); // 预估每个mesh有1000个顶点
	_faces.reserve(model.meshes.size() * 1000);    // 预估每个mesh有1000个面
	for (const auto& mesh : model.meshes) {
		for (const auto& primitive : mesh.primitives) {
			_extract_vertices(model, primitive, _vertices);
			_extract_faces(model, primitive, _faces);
		}
	}
}

Mesh::Mesh(const std::vector<vec3f>& vertices, const std::vector<vec3i>& faces)
	: _vertices(vertices), _faces(faces) {}

auto Mesh::_process_edges() const -> void {
	std::vector<std::pair<int, int>> edge_list;
	edge_list.reserve(_faces.size());
	for (const auto& face : _faces) {
		for (int i = 0; i < 3; ++i) {
			int v1 = face[i];
			int v2 = face[(i + 1) % 3];
			if (v1 > v2) std::swap(v1, v2);
			edge_list.emplace_back(v1, v2);
		}
	}

	std::ranges::sort(edge_list);
	auto tail = std::ranges::unique(edge_list);
	edge_list.erase(tail.begin(), tail.end());

	_edges.reserve(edge_list.size());
	for (const auto& edge : edge_list) {
		_edges.emplace_back(edge.first, edge.second);
	}
}

auto Mesh::_compute_bounds() const -> void {
	for (const auto& vertex : vertices()) {
		_min_bound = _min_bound.cwiseMin(vertex);
		_max_bound = _max_bound.cwiseMax(vertex);
	}
}

auto Mesh::_compute_vertex_degrees() const -> void {
	degree.resize(_vertices.size(), 0);
	for (const auto& edge : edges()) {
		degree[static_cast<size_t>(edge[0])]++;
		degree[static_cast<size_t>(edge[1])]++;
	}
}

} // namespace aiza_dev::mesh_simplify