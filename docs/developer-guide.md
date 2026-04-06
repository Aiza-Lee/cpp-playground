# Developer Guide

This repository hosts multiple independent C++ mini projects. Each top-level folder with a CMakeLists.txt is treated as a standalone project, while root CMake and vcpkg configuration provide shared defaults.

## Project Layout

Typical structure for a new project:

```
<project>/
  CMakeLists.txt
  README.md
  include/<project>/
  src/
  tests/
```

## Create a New Project

Preferred: use the scaffold script:

```
./tools/create_toy_project.sh geometry
```

Common options:

```
./tools/create_toy_project.sh geometry --binary demo
./tools/create_toy_project.sh graph-search --target graph_search_app
```

After scaffolding, reconfigure and build:

```
cmake --preset gcc14-debug
cmake --build --preset gcc14-debug
```

## Shared Dependencies

Common dependencies are managed by vcpkg and wired in cmake/Dependencies.cmake. To add a shared dependency:

1) Add the package to vcpkg.json
2) find_package(...) it in cmake/Dependencies.cmake
3) Link it to cpp_playground::deps

Test-only deps live behind the BUILD_TESTING switch and cpp_playground::test_deps.

## Tests

Configure and build with tests enabled, then run:

```
ctest --preset gcc14-debug --output-on-failure
```

## Notes

- IntelliSense uses clangd (clang++-20), while builds default to g++-14.
- Build presets are defined in CMakePresets.json.
