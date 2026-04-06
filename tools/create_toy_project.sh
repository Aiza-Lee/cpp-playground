#!/usr/bin/env bash
set -euo pipefail

usage() {
	cat <<'EOF'
Usage:
  ./tools/create_toy_project.sh <project-name> [--binary <binary-name>] [--target <target-name>] [--root <dir>]

Examples:
  ./tools/create_toy_project.sh geometry
  ./tools/create_toy_project.sh graph-search --binary demo
  ./tools/create_toy_project.sh graph-search --target graph_search_app

Notes:
  - <project-name> becomes the folder name under the chosen root.
  - By default --root is the repository root.
  - The top-level CMakeLists.txt will auto-discover the generated project.
EOF
}

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd -- "${script_dir}/.." && pwd)"

project_name=""
binary_name=""
target_name=""
root_dir="${repo_root}"

while [[ $# -gt 0 ]]; do
	case "$1" in
		-h|--help)
			usage
			exit 0
			;;
		--binary)
			binary_name="${2:-}"
			shift 2
			;;
		--target)
			target_name="${2:-}"
			shift 2
			;;
		--root)
			root_dir="${2:-}"
			shift 2
			;;
		-*)
			echo "Error: unknown option: $1" >&2
			exit 1
			;;
		*)
			if [[ -n "${project_name}" ]]; then
				echo "Error: project name already provided: ${project_name}" >&2
				exit 1
			fi
			project_name="$1"
			shift
			;;
	esac
done

if [[ -z "${project_name}" ]]; then
	usage
	exit 1
fi

if [[ ! "${project_name}" =~ ^[A-Za-z0-9][A-Za-z0-9_-]*$ ]]; then
	echo "Error: project name must match [A-Za-z0-9][A-Za-z0-9_-]*" >&2
	exit 1
fi

if [[ -z "${binary_name}" ]]; then
	binary_name="${project_name}"
fi

if [[ -z "${target_name}" ]]; then
	target_name="${project_name//-/_}_app"
fi

if [[ ! "${binary_name}" =~ ^[A-Za-z0-9][A-Za-z0-9_-]*$ ]]; then
	echo "Error: binary name must match [A-Za-z0-9][A-Za-z0-9_-]*" >&2
	exit 1
fi

if [[ ! "${target_name}" =~ ^[A-Za-z0-9_][A-Za-z0-9_-]*$ ]]; then
	echo "Error: target name must match [A-Za-z0-9_][A-Za-z0-9_-]*" >&2
	exit 1
fi

project_dir="${root_dir}/${project_name}"
if [[ -e "${project_dir}" ]]; then
	echo "Error: destination already exists: ${project_dir}" >&2
	exit 1
fi

mkdir -p "${project_dir}/src"

cat > "${project_dir}/CMakeLists.txt" <<EOF
cpp_playground_add_executable(
	NAME ${target_name}
	OUTPUT_NAME ${binary_name}
	SOURCES
		src/main.cpp
)
EOF

cat > "${project_dir}/README.md" <<EOF
# ${project_name}

Describe what this toy project explores.

## Build

\`\`\`bash
cmake --preset gcc14-debug
cmake --build --preset gcc14-debug
./build/gcc14-debug/bin/${project_name}/${binary_name}
\`\`\`
EOF

cat > "${project_dir}/src/main.cpp" <<EOF
#include <iostream>

int main() {
	std::cout << "Hello from ${project_name}" << '\\n';
	return 0;
}
EOF

echo "Created toy project at: ${project_dir}"
echo "Target name: ${target_name}"
echo "Binary name: ${binary_name}"
echo "Next step: cmake --preset gcc14-debug && cmake --build --preset gcc14-debug"
