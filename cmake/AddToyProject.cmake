# 仅加载一次，避免重复定义
include_guard(GLOBAL)

# 使用 CMake 内置的参数解析工具
include(CMakeParseArguments)

# 添加可执行程序目标的统一封装
function(cpp_playground_add_executable)
	# 解析函数参数
	set(options)
	set(one_value_args NAME OUTPUT_NAME)
	set(multi_value_args SOURCES INCLUDE_DIRECTORIES LINK_LIBRARIES COMPILE_DEFINITIONS)
	cmake_parse_arguments(APP "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

	# 参数校验
	if(NOT APP_NAME)
		message(FATAL_ERROR "cpp_playground_add_executable requires NAME.")
	endif()

	# 参数校验
	if(NOT APP_SOURCES)
		message(FATAL_ERROR "cpp_playground_add_executable requires at least one source file.")
	endif()

	# 创建可执行文件并应用默认规则
	add_executable(${APP_NAME} ${APP_SOURCES})
	cpp_playground_apply_defaults(${APP_NAME})
	cpp_playground_link_common_deps(${APP_NAME})

	# 可选 include 目录
	if(APP_INCLUDE_DIRECTORIES)
		target_include_directories(${APP_NAME} PRIVATE ${APP_INCLUDE_DIRECTORIES})
	endif()

	# 可选链接库
	if(APP_LINK_LIBRARIES)
		target_link_libraries(${APP_NAME} PRIVATE ${APP_LINK_LIBRARIES})
	endif()

	# 可选编译宏定义
	if(APP_COMPILE_DEFINITIONS)
		target_compile_definitions(${APP_NAME} PRIVATE ${APP_COMPILE_DEFINITIONS})
	endif()

	# 设置 IDE 分组与输出目录
	get_filename_component(current_folder "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
	set_target_properties(${APP_NAME}
		PROPERTIES
			FOLDER "${current_folder}"
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${current_folder}")

	# 可选输出文件名
	if(APP_OUTPUT_NAME)
		set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME "${APP_OUTPUT_NAME}")
	endif()
endfunction()

# 添加测试目标的统一封装
function(cpp_playground_add_test)
	# 未启用测试时直接返回
	if(NOT BUILD_TESTING)
		return()
	endif()

	# 解析函数参数
	set(options)
	set(one_value_args NAME)
	set(multi_value_args SOURCES INCLUDE_DIRECTORIES LINK_LIBRARIES COMPILE_DEFINITIONS)
	cmake_parse_arguments(TEST "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

	# 参数校验
	if(NOT TEST_NAME)
		message(FATAL_ERROR "cpp_playground_add_test requires NAME.")
	endif()

	# 参数校验
	if(NOT TEST_SOURCES)
		message(FATAL_ERROR "cpp_playground_add_test requires at least one source file.")
	endif()

	# 创建测试可执行文件并应用默认规则
	add_executable(${TEST_NAME} ${TEST_SOURCES})
	cpp_playground_apply_defaults(${TEST_NAME})
	cpp_playground_link_test_deps(${TEST_NAME})

	# 可选 include 目录
	if(TEST_INCLUDE_DIRECTORIES)
		target_include_directories(${TEST_NAME} PRIVATE ${TEST_INCLUDE_DIRECTORIES})
	endif()

	# 可选链接库
	if(TEST_LINK_LIBRARIES)
		target_link_libraries(${TEST_NAME} PRIVATE ${TEST_LINK_LIBRARIES})
	endif()

	# 可选编译宏定义
	if(TEST_COMPILE_DEFINITIONS)
		target_compile_definitions(${TEST_NAME} PRIVATE ${TEST_COMPILE_DEFINITIONS})
	endif()

	# 设置 IDE 分组与输出目录
	get_filename_component(current_folder "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
	set_target_properties(${TEST_NAME}
		PROPERTIES
			FOLDER "${current_folder}"
			RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${current_folder}/tests")

	# 注册到 CTest
	add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()
