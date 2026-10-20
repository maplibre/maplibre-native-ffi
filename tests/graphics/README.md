# Test graphics

`mln_test_graphics` creates the GPU objects that a host would otherwise bring: a
device or context, a borrowed texture, and a presentation surface, for Metal,
Vulkan, EGL, and WGL. The C suite and every binding suite take their GPU objects
from here, so tests never create them by hand. WebGL and WebGPU contexts come
from JavaScript, so the browser builds keep their own fixtures.

[`include/mln_test_graphics.h`](include/mln_test_graphics.h) is the whole
interface, and its comments state each function's contract.

## Build

`graphics.c` is one C11 file. With `BUILD_TESTING` on, outside Emscripten, CMake
builds it twice:

- `mln_test_graphics_objects` links into the C suite's executable.
- `mln_test_graphics` is a shared library for the binding suites.
  `mise run build` installs it into `build/<preset>/install` with its header and
  `share/pkgconfig/mln-test-graphics.pc`, through the `test-graphics` component.
  That component stays out of a full installation and out of the package.

A binding that compiles `graphics.c` itself adds MapLibre Native's
`vendor/Vulkan-Headers/include` to its include path, and links `dl` on Linux, or
`user32` and `gdi32` on Windows. A binding that tests on a device pushes the
library beside its test executables.

## Load it from a binding

| Binding | Where it loads the library                                        |
| ------- | ----------------------------------------------------------------- |
| Go      | `internal/testsupport/graphics.go`, through pkg-config            |
| Swift   | The `GraphicsSupport` SwiftPM target, which compiles `graphics.c` |
| Kotlin  | `TestGraphicsLibrary.<target>.kt`: FFM, cinterop, or a JNI shim   |
| Rust    | `tests/suite/support/graphics.rs`, through `libloading`           |
| Zig     | The root `build.zig`, which translates the header                 |
| Python  | `tests/graphics.py`, through `ctypes`                             |
| .NET    | `Support/TestGraphics.cs`, through `LibraryImport`                |
| Dart    | `test/support/graphics.dart`, through `DynamicLibrary`            |
