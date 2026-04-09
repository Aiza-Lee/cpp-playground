#pragma once

#include "mesh_simplify/mesh.hpp"

namespace aiza_dev::mesh_simplify::render {

class GLMesh {

public:
	GLMesh(const GLMesh&) = delete;
	GLMesh(GLMesh&&) = delete;
	GLMesh& operator=(const GLMesh&) = delete;
	GLMesh& operator=(GLMesh&&) = delete;

	GLMesh(const Mesh& mesh);
	~GLMesh();
	void draw() const;
	void draw_line_frame() const;

private:
	u32 _vao{0};			// 顶点数组对象
	u32 _vbo{0};			// 顶点缓冲对象
	u32 _ebo{0};			// 索引缓冲对象
	u32 _vertex_count{0};	// 顶点数量

	u32 _color_vbo{0};		// 颜色缓冲对象
};

} // namespace aiza_dev::mesh_simplify::render