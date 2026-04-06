# 仅加载一次，避免重复定义
include_guard(GLOBAL)

# 基础依赖（始终需要）
find_package(cxxopts CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# 测试依赖（仅在 BUILD_TESTING=ON 时需要）
if(BUILD_TESTING)
	find_package(doctest CONFIG REQUIRED)
endif()

# 通用依赖的接口库
add_library(cpp_playground_deps INTERFACE)
add_library(cpp_playground::deps ALIAS cpp_playground_deps)
target_link_libraries(cpp_playground_deps
	INTERFACE
		cxxopts::cxxopts
		fmt::fmt-header-only
		nlohmann_json::nlohmann_json
		spdlog::spdlog_header_only)

# 测试依赖的接口库
add_library(cpp_playground_test_deps INTERFACE)
add_library(cpp_playground::test_deps ALIAS cpp_playground_test_deps)
if(BUILD_TESTING)
	target_link_libraries(cpp_playground_test_deps INTERFACE doctest::doctest)
endif()

# 默认依赖说明（便于阅读）
# Shared libs available to all toy executables by default:
# - fmt: formatting
# - spdlog: logging
# - nlohmann_json: JSON support
# - cxxopts: lightweight CLI parsing
#
# Test-only deps stay separate and can be opted in via
# cpp_playground_link_test_deps(...).

# 为目标链接通用依赖
function(cpp_playground_link_common_deps target_name)
	target_link_libraries(${target_name} PRIVATE cpp_playground::deps)
endfunction()

# 为目标链接测试依赖
function(cpp_playground_link_test_deps target_name)
	target_link_libraries(${target_name} PRIVATE cpp_playground::test_deps)
endfunction()
