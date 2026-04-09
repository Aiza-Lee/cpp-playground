#include <glad/glad.h>
#include <ranges>
#include <algorithm>

#include "mesh_simplify/render/gl-mesh.hpp"

namespace aiza_dev::mesh_simplify::render {

namespace {

auto _get_color_by_degree(int degree) -> vec3f {
    float t = static_cast<float>(degree - 2) / 8.0F; 
    t = std::clamp(t, 0.0F, 1.0F);

    // 蓝色 (0.2, 0.2, 0.8) -> 红色 (0.8, 0.2, 0.2)
    float r = 0.2F + (t * 0.6F);         // 随着度数增加，R通道从0.2涨到0.8
    float g = 0.2F;
	float b = 0.8F - (t * 0.6F);         // 随着度数增加，B通道从0.8降到0.2

    return vec3f{r, g, b};
}

}

GLMesh::GLMesh(const Mesh& mesh) {
	_vertex_count = static_cast<u32>(mesh.faces().size() * 3);

	glGenVertexArrays(1, &_vao);
	glGenBuffers(1, &_vbo);
	glGenBuffers(1, &_ebo);
	glGenBuffers(1, &_color_vbo);

	glBindVertexArray(_vao);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		static_cast<GLsizeiptr>(mesh.vertices().size() * sizeof(vec3f)), 
		mesh.vertices().data(), 
		GL_STATIC_DRAW
	);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		static_cast<GLsizeiptr>(mesh.faces().size() * sizeof(vec3i)), 
		mesh.faces().data(), 
		GL_STATIC_DRAW
	);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 3, 
		GL_FLOAT, GL_FALSE, 
		sizeof(vec3f), 
		nullptr
	);


	glBindBuffer(GL_ARRAY_BUFFER, _color_vbo);
	auto color_view = mesh.vertex_degrees() | std::views::transform([](int deg) { return _get_color_by_degree(deg); });
	auto color = std::vector<vec3f>(color_view.begin(), color_view.end());
	glBufferData(
		GL_ARRAY_BUFFER, 
		static_cast<GLsizeiptr>(color.size() * sizeof(vec3f)), 
		color.data(), 
		GL_STATIC_DRAW
	);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 3,
		GL_FLOAT, GL_FALSE,
		sizeof(vec3f),
		nullptr
	);

	glBindVertexArray(0);
}

GLMesh::~GLMesh() {
	glDeleteVertexArrays(1, &_vao);
	glDeleteBuffers(1, &_vbo);
	glDeleteBuffers(1, &_ebo);
	glDeleteBuffers(1, &_color_vbo);
}

auto GLMesh::draw() const -> void {
	glBindVertexArray(_vao);
	glDrawElements(
		GL_TRIANGLES, 
		static_cast<int>(_vertex_count), 
		GL_UNSIGNED_INT, 
		nullptr
	);
	glBindVertexArray(0);
}

auto GLMesh::draw_line_frame() const -> void {
	glBindVertexArray(_vao);
	glDrawElements(
		GL_LINES, 
		static_cast<int>(_vertex_count), 
		GL_UNSIGNED_INT, 
		nullptr
	);
	glBindVertexArray(0);
}

} // namespace aiza_dev::mesh_simplify::render