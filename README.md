# C++ Playground

轻量的 C++ 实验项目集合：每个一级子目录是独立项目，顶层统一 CMake/vcpkg 配置。编辑器用 `clang++-20` 做 IntelliSense，构建默认 `g++-14`。

## 编译

```bash
export VCPKG_ROOT=/path/to/vcpkg
cmake --preset gcc14-debug
cmake --build --preset gcc14-debug
```

不需要测试依赖时：

```bash
cmake --preset gcc14-debug-no-tests
cmake --build --preset gcc14-debug-no-tests
```

更多开发细节见 [docs/developer-guide.md](docs/developer-guide.md)。
