# About

AnyBox is a compatibility layer for running Xbox Series X|S executables natively on Windows. Part of [AnyPS5](https://github.com/boykopovar/AnyPS5) ecosystem. The [shader recompiler from AnyPS5 core](https://github.com/boykopovar/AnyPS5/tree/main/core/shader/recompiler) is planned to be used.

## Status

Initial development. No playable titles confirmed.

Seven system libraries with a total of 39 functions required by the test title and its bundled libraries have been added and build successfully. Launch has been tested with the title's executable.

Unimplemented functions throw a C++ exception. Cross-library stack unwinding and destructor execution are verified by tests. The title reaches an unimplemented function and terminates with an unhandled MSVC-compatible C++ exception; its call stack is visible in the debugger. Destructor execution during unwinding through the title's code has not been verified.

See [Checking PE imports](tools/README.md) to compare an executable or companion DLL with the exports in a local AnyBox build.

## Building

AnyBox uses standard C++20 and can be built with any conforming compiler targeting 64-bit Windows. MSVC or `clang-cl` with the MSVC runtime is recommended for ABI compatibility and correct stack unwinding of diagnostic C++ exceptions through title code.

## Disclaimer

The project is intended for interoperability, research and preservation. Game files, firmware, cryptographic keys and proprietary SDK libraries are not distributed with the project.

AnyBox is an independent project and is not affiliated with, authorized or endorsed by Microsoft. Xbox is a trademark of Microsoft Corporation.

## License

This project is licensed under the [GNU General Public License version 2 only](LICENSE).
