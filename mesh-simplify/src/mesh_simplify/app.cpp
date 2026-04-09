
#include <Eigen/Geometry>
#include <fmt/ostream.h>
#include <fmt/base.h>

#include "mesh_simplify/base.hpp"
#include "mesh_simplify/app.hpp"
#include "mesh_simplify/mesh.hpp"
#include "mesh_simplify/render/gl-mesh.hpp"
#include "mesh_simplify/render/camera.hpp"
#include "mesh_simplify/shader-program.hpp"
#include "mesh_simplify/model-loader.hpp"

namespace aiza_dev::mesh_simplify {

auto App::run(const fpath& model_path, const fpath& vertex_shader_path, const fpath& fragment_shader_path) -> void{
	fmt::print("Hello from mesh-simplify\n");

	auto model = ModelLoader::load_model(model_path);

	auto shader_program = ShaderProgram(vertex_shader_path, fragment_shader_path);

	auto mesh = Mesh(model);
	auto gl_mesh = render::GLMesh(mesh);

	Eigen::Affine3f model_transform = Eigen::Affine3f::Identity();
	model_transform.scale(8.0F);

	_window.set_clear_color({1.0F, 1.0F, 1.0F, 1.0F});
	
	render::Camera camera;
	const vec3f camera_position{0.0F, 0.0F, 2.0F};
	const vec3f camera_target{0.0F, 0.3F, 0.0F};
	const vec3f camera_up{0.0F, 1.0F, 0.0F};
	camera.look_at(camera_position, camera_target, camera_up);
	camera.set_near_plane(0.1F);
	camera.set_far_plane(100.0F);

	_window.register_window_resize_callback([&](int width, int height) {
		camera.set_aspect_ratio(static_cast<float>(width) / static_cast<float>(height));
	});

	bool once = true;

	_window.register_update([&]() {
		shader_program.use();
		mat4f mvp = camera.get_view_projection_matrix() * model_transform.matrix();
		shader_program.set_uniform("uMVP", mvp);
		
		if (once) {
			auto mat = camera.get_view_projection_matrix();
			fmt::print("Camera View-Projection Matrix:\n");
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					fmt::print("{:>8.3f} ", mat(i, j));
				}
				fmt::print("\n");
			}
			once = false;
		}

		gl_mesh.draw_line_frame();
		model_transform.rotate(Eigen::AngleAxisf(0.01F, vec3f{0.0F, 1.0F, 0.0F}));
	});

	while (!_window.should_close()) {
		_window.update();
	}	
}

} // namespace aiza_dev::mesh_simplify