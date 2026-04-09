find_package(Eigen3 CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
add_library(tinygltf_headers INTERFACE)
target_include_directories(tinygltf_headers INTERFACE ${TINYGLTF_INCLUDE_DIRS})

add_library(stb_headers INTERFACE)
target_include_directories(stb_headers INTERFACE ${Stb_INCLUDE_DIR})

set(mesh_simplify_sources
	src/mesh_simplify/render/camera.cpp
	src/mesh_simplify/render/gl-mesh.cpp
	src/mesh_simplify/utils/utils.cpp
	src/mesh_simplify/app.cpp
	src/mesh_simplify/mesh.cpp
	src/mesh_simplify/model-loader.cpp
	src/mesh_simplify/shader-program.cpp
	src/mesh_simplify/tinygltf.cpp
	src/mesh_simplify/window.cpp
)

add_library(mesh_simplify_lib STATIC ${mesh_simplify_sources})

cpp_playground_apply_defaults(mesh_simplify_lib)
cpp_playground_link_common_deps(mesh_simplify_lib)

target_include_directories(mesh_simplify_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(mesh_simplify_lib PUBLIC
	Eigen3::Eigen
	OpenGL::GL
	tinygltf_headers
	glfw
	glad::glad
)
	
target_link_libraries(mesh_simplify_lib PRIVATE
	stb_headers
)

cpp_playground_add_executable(
	NAME mesh_simplify
	OUTPUT_NAME mesh-simplify
	SOURCES
		src/main.cpp
	LINK_LIBRARIES
		mesh_simplify_lib
)

target_compile_definitions(mesh_simplify PRIVATE "MESH_SIMPLIFY_ASSETS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets\"")
