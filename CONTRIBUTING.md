# Contributing to AnyBox

## Build and test on Windows

Use CMake 3.25 or newer and a C++20 compiler targeting x64 Windows. MSVC or `clang-cl` with the MSVC runtime is recommended for testing exception unwinding through title code.

From the repository root, configure with Visual Studio 2022 and the x64 C++ tools installed.

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
```

For a MinGW GCC build, put the x64 MinGW compiler and Ninja on `PATH`, then configure a separate build directory.

```powershell
cmake -S . -B build-mingw -G "Ninja Multi-Config" -DCMAKE_CXX_COMPILER=g++ -DBUILD_TESTING=ON
```

Set `$build` to the directory you configured. Build and run the tests in both configurations.

```powershell
$build = 'build-msvc'
cmake --build $build --config Debug --parallel
ctest --test-dir $build -C Debug --output-on-failure --no-tests=error
cmake --build $build --config Release --parallel
ctest --test-dir $build -C Release --output-on-failure --no-tests=error
```

The CTest suite runs without game files. Build output goes to `bin/<configuration>` under the build directory.

## Report a bug or submit a change

Search existing issues and pull requests for the same problem. In the bug report, identify the AnyBox revision, Windows and compiler versions, and the command, test, or launch step that reproduces the behavior. Include the first relevant error or missing export if one is available.

For a pull request, describe the behavior being changed and report what you verified, including the Windows version, compiler, build configuration, and test result. A small synthetic reproduction or regression test is useful when it exercises the changed behavior without game files.
