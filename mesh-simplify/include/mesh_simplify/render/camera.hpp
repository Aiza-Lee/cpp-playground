#include "mesh_simplify/base.hpp"

namespace aiza_dev::mesh_simplify::render {

class Camera {

public:
	Camera() = default;
	Camera(const Camera&) = default;
	Camera(Camera&&) = delete;
	Camera& operator=(const Camera&) = default;
	Camera& operator=(Camera&&) = delete;
	~Camera() = default;

	[[nodiscard]] auto get_view_matrix() const -> mat4f;
	[[nodiscard]] auto get_projection_matrix() const -> mat4f;
	[[nodiscard]] auto get_view_projection_matrix() const -> mat4f;

	[[nodiscard]] auto get_position() const -> vec3f;
	auto set_position(const vec3f& position) -> void;

	[[nodiscard]] auto get_target() const -> vec3f;
	auto set_target(const vec3f& target) -> void;

	[[nodiscard]] auto get_up() const -> vec3f;
	auto set_up(const vec3f& up) -> void;

	[[nodiscard]] auto get_fov() const -> float;
	auto set_fov(float fov) -> void;
	
	[[nodiscard]] auto get_aspect_ratio() const -> float;
	auto set_aspect_ratio(float aspect_ratio) -> void;

	[[nodiscard]] auto get_near_plane() const -> float;
	auto set_near_plane(float near_plane) -> void;

	[[nodiscard]] auto get_far_plane() const -> float;
	auto set_far_plane(float far_plane) -> void;


	auto look_at(const vec3f& position, const vec3f& target, const vec3f& up) -> void;

private:
	vec3f _position{0.0F, 0.0F, 5.0F};
	vec3f _target{0.0F, 0.0F, 0.0F};
	vec3f _up{0.0F, 1.0F, 0.0F};
	float _fov{45.0F};
	float _aspect_ratio{1.0F};
	float _near_plane{0.1F};
	float _far_plane{100.0F};
};

} // namespace aiza_dev::mesh_simplify::render