# 仅加载一次，避免重复定义
include_guard(GLOBAL)

# Sanitizer 开关
option(CPP_PLAYGROUND_ENABLE_ASAN "Enable AddressSanitizer for playground targets." OFF)
option(CPP_PLAYGROUND_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for playground targets." OFF)

# 基础编译选项接口库
add_library(cpp_playground_options INTERFACE)
add_library(cpp_playground::options ALIAS cpp_playground_options)

# 统一 C++ 标准
target_compile_features(cpp_playground_options INTERFACE cxx_std_23)

# 警告选项接口库
add_library(cpp_playground_warnings INTERFACE)
add_library(cpp_playground::warnings ALIAS cpp_playground_warnings)

# 按编译器设置警告等级
target_compile_options(cpp_playground_warnings
	INTERFACE
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wpedantic>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wconversion>
		$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wsign-conversion>
		$<$<CXX_COMPILER_ID:MSVC>:/W4>
		$<$<CXX_COMPILER_ID:MSVC>:/permissive->)

# Sanitizer 选项接口库
add_library(cpp_playground_sanitizers INTERFACE)
add_library(cpp_playground::sanitizers ALIAS cpp_playground_sanitizers)

# 按需启用 ASan/UBSan
if(CPP_PLAYGROUND_ENABLE_ASAN OR CPP_PLAYGROUND_ENABLE_UBSAN)
	# 收集要启用的 sanitizer 名称
	set(_sanitizers "")
	if(CPP_PLAYGROUND_ENABLE_ASAN)
		list(APPEND _sanitizers "address")
	endif()
	if(CPP_PLAYGROUND_ENABLE_UBSAN)
		list(APPEND _sanitizers "undefined")
	endif()
	list(JOIN _sanitizers "," _sanitize_flag)

	# 编译阶段的 sanitizer 选项
	target_compile_options(cpp_playground_sanitizers
		INTERFACE
			$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=${_sanitize_flag}>
			$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-omit-frame-pointer>)
	# 链接阶段的 sanitizer 选项
	target_link_options(cpp_playground_sanitizers
		INTERFACE
			$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=${_sanitize_flag}>)
endif()

# 将统一选项应用到指定目标
function(cpp_playground_apply_defaults target_name)
	target_link_libraries(${target_name}
		PRIVATE
			cpp_playground::options
			cpp_playground::warnings
			cpp_playground::sanitizers)
endfunction()
