#include <stdexcept>

#include <glad/glad.h>

#include "mesh_simplify/window.hpp"

namespace aiza_dev::mesh_simplify {

Window::Window(int width, int height, std::string title) : _width(width), _height(height), _title(std::move(title)) {
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialize GLFW");
	}

	_window = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
	if (_window == nullptr) {
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(_window);
	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, _window_resize_callback);

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glEnable(GL_DEPTH_TEST);
	glLineWidth(1.0F);
	glClearColor(_clear_color.x(), _clear_color.y(), _clear_color.z(), _clear_color.w());
}

Window::~Window() {
	glfwDestroyWindow(_window);
	glfwTerminate();
}

auto Window::should_close() const -> bool {
	return glfwWindowShouldClose(_window);
}

auto Window::update() -> void {

	_process_input();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	for (const auto& func : _update_registers) {
		func();
	}

	glfwSwapBuffers(_window);
	glfwPollEvents();
}

auto Window::register_update(std::function<void()> func) -> void {
	_update_registers.push_back(std::move(func));
}
auto Window::register_window_resize_callback(std::function<void(int, int)> func) -> void {
	_resize_registers.push_back(std::move(func));
}

auto Window::set_clear_color(vec4f color) -> void {
	_clear_color = color;
	glClearColor(color.x(), color.y(), color.z(), color.w());
}

auto Window::_process_input() -> void {
	if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(_window, true);
	}
}

auto Window::_window_resize_callback(GLFWwindow* window, int width, int height) -> void {
	glViewport(0, 0, width, height);

	auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	for (const auto& func : self->_resize_registers) {
		func(width, height);
	}
}

} // namespace aiza_dev::mesh_simplify