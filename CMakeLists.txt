# CMake 最低版本要求
cmake_minimum_required(VERSION 3.28)

# 若未显式指定 toolchain，则自动接入 vcpkg
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
	set(CMAKE_TOOLCHAIN_FILE
		"$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
		CACHE FILEPATH "vcpkg toolchain file")
endif()

# 统一测试开关
option(BUILD_TESTING "Build tests for playground targets." ON)

# 同步 vcpkg features：只在需要时启用 tests
set(_cpp_playground_vcpkg_features "")
if(DEFINED VCPKG_MANIFEST_FEATURES AND NOT VCPKG_MANIFEST_FEATURES STREQUAL "")
	set(_cpp_playground_vcpkg_features ${VCPKG_MANIFEST_FEATURES})
endif()
list(REMOVE_ITEM _cpp_playground_vcpkg_features "tests")
if(BUILD_TESTING)
	list(APPEND _cpp_playground_vcpkg_features "tests")
endif()
set(VCPKG_MANIFEST_FEATURES "${_cpp_playground_vcpkg_features}"
	CACHE STRING "vcpkg manifest features" FORCE)

# 项目声明
project(cpp_playground VERSION 0.1.0 LANGUAGES CXX)

# 全局编译与生成设置
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 追加自定义模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 引入公共模块
include(ProjectOptions)
include(CTest)
include(Dependencies)
include(AddToyProject)

# 清理临时变量
unset(_cpp_playground_vcpkg_features)

# 自动发现一级子项目并加入构建
file(GLOB playground_dirs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*")
foreach(dir_name IN LISTS playground_dirs)
	# 仅处理目录
	if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}")
		continue()
	endif()

	# 跳过隐藏目录
	if(dir_name MATCHES "^\\.")
		continue()
	endif()

	# 跳过构建与工具目录
	if(dir_name STREQUAL "build" OR dir_name STREQUAL "cmake" OR dir_name STREQUAL "vcpkg_installed")
		continue()
	endif()

	# 发现子项目后加入
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir_name}/CMakeLists.txt")
		message(STATUS "Adding toy project: ${dir_name}")
		add_subdirectory("${dir_name}")
	endif()
endforeach()
