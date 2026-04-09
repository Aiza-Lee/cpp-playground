#pragma once

#include "mesh_simplify/base.hpp"
#include <functional>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace aiza_dev::mesh_simplify {

class Window {
public:
	Window() = delete;
	Window& operator=(const Window&) = delete;
	Window& operator=(Window&&) = delete;
	Window(int width, int height, std::string title);
	Window(const Window&) = delete;
	Window(Window&&) = delete;
	~Window();

	[[nodiscard]] auto should_close() const -> bool;
	auto update() -> void;
	auto register_update(std::function<void()> func) -> void;
	auto register_window_resize_callback(std::function<void(int, int)> func) -> void;
	
	auto set_clear_color(vec4f color) -> void;

private:
	GLFWwindow* _window;
	int _width; 
	int _height;
	std::string _title;
	std::vector<std::function<void()>> _update_registers;
	std::vector<std::function<void(int, int)>> _resize_registers;
	vec4f _clear_color{0.1F, 0.1F, 0.1F, 1.0F};

	auto _process_input() -> void;
	static auto _window_resize_callback(GLFWwindow* window, int width, int height) -> void;
};

} // namespace aiza_dev::mesh_simplify