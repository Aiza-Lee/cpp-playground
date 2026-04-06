# 核心实现源文件列表
set(automata_nfa_dfa_sources
	src/nfa_dfa/algorithm.cpp
	src/nfa_dfa/app.cpp
	src/nfa_dfa/model.cpp
	src/nfa_dfa/output.cpp
	src/nfa_dfa/parser.cpp)

# 核心库目标（便于复用与测试）
add_library(automata_nfa_dfa_lib STATIC ${automata_nfa_dfa_sources})

# 应用统一编译选项与公共依赖
cpp_playground_apply_defaults(automata_nfa_dfa_lib)
cpp_playground_link_common_deps(automata_nfa_dfa_lib)

# 对外暴露 include 目录
target_include_directories(automata_nfa_dfa_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

# IDE 里的分组显示
set_target_properties(automata_nfa_dfa_lib PROPERTIES FOLDER "automata")

# 可执行程序目标
cpp_playground_add_executable(
	NAME automata_nfa_dfa
	OUTPUT_NAME nfa_dfa
	SOURCES
		src/nfa_dfa/main.cpp
	LINK_LIBRARIES
		automata_nfa_dfa_lib)

# 测试目标（按 BUILD_TESTING 开关控制）
if(BUILD_TESTING)
	cpp_playground_add_test(
		NAME automata_nfa_dfa_tests
		SOURCES
			tests/nfa_dfa_tests.cpp
		INCLUDE_DIRECTORIES
			"${CMAKE_CURRENT_SOURCE_DIR}/include"
		LINK_LIBRARIES
			automata_nfa_dfa_lib)
endif()
