#include <numbers>

#include <Eigen/Geometry>

#include "mesh_simplify/render/camera.hpp"
#include "mesh_simplify/base.hpp"

namespace aiza_dev::mesh_simplify::render {

auto Camera::get_position() const -> vec3f { return _position; }
auto Camera::set_position(const vec3f& position) -> void { _position = position; }
auto Camera::get_target() const -> vec3f { return _target; }
auto Camera::set_target(const vec3f& target) -> void { _target = target; }
auto Camera::get_up() const -> vec3f { return _up; }
auto Camera::set_up(const vec3f& up) -> void { _up = up; }
auto Camera::get_fov() const -> float { return _fov; }
auto Camera::set_fov(float fov) -> void { _fov = fov; }
auto Camera::get_aspect_ratio() const -> float { return _aspect_ratio; }
auto Camera::set_aspect_ratio(float aspect_ratio) -> void { _aspect_ratio = aspect_ratio; }
auto Camera::get_near_plane() const -> float { return _near_plane; }
auto Camera::set_near_plane(float near_plane) -> void { _near_plane = near_plane; }
auto Camera::get_far_plane() const -> float { return _far_plane; }
auto Camera::set_far_plane(float far_plane) -> void { _far_plane = far_plane; }

auto Camera::look_at(const vec3f& position, const vec3f& target, const vec3f& up) -> void {
	_position = position;
	_target = target;
	_up = up;
}

auto Camera::get_view_matrix() const -> mat4f {
	vec3f zaxis = (_position - _target).normalized();
	vec3f xaxis = _up.cross(zaxis).normalized();
	vec3f yaxis = zaxis.cross(xaxis).normalized();

	mat4f view;

	view << xaxis.x(), xaxis.y(), xaxis.z(), -xaxis.dot(_position),
			yaxis.x(), yaxis.y(), yaxis.z(), -yaxis.dot(_position),
			zaxis.x(), zaxis.y(), zaxis.z(), -zaxis.dot(_position),
			0.0F, 0.0F, 0.0F, 1.0F;
	return view;
}

auto Camera::get_projection_matrix() const -> mat4f {
	
	auto tan_half_fov = std::tan(_fov * 0.5F * std::numbers::pi_v<float> / 180.0F);

	mat4f projection;
	projection << 1.0F / (_aspect_ratio * tan_half_fov), 0.0F, 0.0F, 0.0F,
				  0.0F, 1.0F / tan_half_fov, 0.0F, 0.0F,
				  0.0F, 0.0F, -(_far_plane + _near_plane) / (_far_plane - _near_plane), -2.0F * _far_plane * _near_plane / (_far_plane - _near_plane),
				  0.0F, 0.0F, -1.0F, 0.0F;
	return projection;
}

auto Camera::get_view_projection_matrix() const -> mat4f {
	return get_projection_matrix() * get_view_matrix();
}

} // namespace aiza_dev::mesh_simplify::render