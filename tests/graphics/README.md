# Test graphics

`mln_test_graphics` creates the GPU objects that a host would otherwise bring: a
device or context, a borrowed texture, and a presentation surface, in the shapes
that the render target descriptors take. The C suite and the binding suites take
their GPU objects from here, so one implementation stands in for the host on
every backend.

| Backend | Context                                                  | Borrowed texture                     | Surface                                                   |
| ------- | -------------------------------------------------------- | ------------------------------------ | --------------------------------------------------------- |
| Metal   | The system default `MTLDevice`                           | A BGRA8 `MTLTexture`                 | A `CAMetalLayer` with no window                           |
| Vulkan  | An instance, device, and graphics queue on the first GPU | An RGBA8 `VkImage` and `VkImageView` | A Metal-layer, Win32, Android, or headless `VkSurfaceKHR` |
| EGL     | An OpenGL ES 3 context on a pbuffer config               | An RGBA8 texture in that context     | A pbuffer                                                 |
| WGL     | A context on the device context of a hidden window       | An RGBA8 texture in that context     | The device context of another hidden window               |

An Android Vulkan surface presents into the window of an `AImageReader` from
`libmediandk.so`, which discards each frame it receives, so a test presents
without a view or an activity.

WebGL and WebGPU have no entry here. Their contexts come from JavaScript in the
browser build, so the browser suite keeps its own fixtures in
`tests/native/support`, and Rust's browser tests keep `webgl_gl.rs`.

## Interface

`include/mln_test_graphics.h` is the whole interface. Its comments state each
function's contract.

- `mln_test_graphics_create(backend)` returns a graphics object, or null with
  the reason in `mln_test_graphics_last_error()`. The backend is one of the
  `MLN_TEST_GRAPHICS_BACKEND_*` values.
- `mln_test_graphics_get_context()` fills a `mln_test_graphics_context` with the
  handles that the backend's context descriptor takes.
- `mln_test_graphics_make_current()` makes an EGL or WGL context current on the
  calling thread, for a session that the host drives from that thread.
- `mln_test_graphics_texture_create()` and `mln_test_graphics_surface_create()`
  create the targets that borrowed texture and surface descriptors name. Their
  `get_info` functions return the handles.
- `mln_test_graphics_texture_read_rgba8()` copies a borrowed texture's pixels to
  memory, so a test can check what a session rendered into it. It shares the
  core's queue or context unsynchronized, so a test reads directly after a
  fence, such as a finished render barrier.

The library loads each graphics API at run time and links none. It looks for the
Vulkan loader in `MLN_FFI_VULKAN_LOADER_DIR` first, then at the path that the
build found, then through the platform's search. EGL and OpenGL ES follow the
same order without the variable. A build therefore runs wherever the loader is
present, and a failure to find one arrives as an error message rather than as a
load failure.

A graphics object and what it creates belong to one thread at a time. Destroy
textures, surfaces, and every session that borrows them before the graphics
object. The library never calls `eglTerminate()`, because a display is shared by
everything in the process that uses it.

## Build

`graphics.c` is one C11 file with no generated inputs. CMake builds it twice
when `BUILD_TESTING` is on, except for Emscripten:

- `mln_test_graphics_objects` links into the C suite's executable, so the
  emulator and simulator runners push nothing extra.
- `mln_test_graphics` is a shared library for the binding suites.
  `mise run build` installs it with its header and a pkg-config file through the
  `test-graphics` component. That component stays out of a full installation and
  out of the package. `mise run install-native-package`, which replaces the
  install directory with a package, carries the component's files over, so the
  binding suites that CI runs against a package still find them.

A preset's install directory then holds:

| Path                                   | Contents               |
| -------------------------------------- | ---------------------- |
| `lib/libmln_test_graphics.{so,dylib}`  | The library on Unix    |
| `bin/mln_test_graphics.dll`            | The library on Windows |
| `include/mln_test_graphics.h`          | The interface          |
| `share/pkgconfig/mln-test-graphics.pc` | Compile and link flags |

The Vulkan code compiles against the Vulkan headers that MapLibre Native
vendors, `third_party/maplibre-native/vendor/Vulkan-Headers/include`. A consumer
that compiles `graphics.c` itself adds that directory to its include path, and
links `dl` on Linux, or `user32` and `gdi32` on Windows.

## Loading it from a binding

Each binding loads the library the way it loads `maplibre-native-c`, from
`build/<preset>/install`. The handles come back as pointers and 64-bit integers,
which the binding passes to its own descriptor types. The rows marked planned
describe the mechanism a binding adopts when its GPU fixture moves here; until
then, that binding keeps its own fixture.

| Binding        | Status  | How it loads the library                                                                                                                                   |
| -------------- | ------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| C              | In use  | Links `mln_test_graphics_objects` in CMake.                                                                                                                |
| Go             | In use  | `#cgo pkg-config: mln-test-graphics` in `internal/testsupport`, with `PKG_CONFIG_PATH` at the install's `share/pkgconfig`.                                 |
| Swift          | In use  | The `GraphicsSupport` SwiftPM target, which compiles `graphics.c` from this directory.                                                                     |
| Kotlin/Native  | In use  | A cinterop definition over `mln_test_graphics.h`, linking `graphics.c` compiled by Gradle.                                                                 |
| Kotlin/Android | In use  | A JNI shim compiled with `graphics.c` by the NDK, loaded with `System.loadLibrary`.                                                                        |
| Rust           | In use  | `libloading` in `tests/suite/support/graphics.rs`, from the install directory that `build.rs` records, or by name beside the tests a device runner pushes. |
| Zig            | In use  | `build.zig` translates `mln_test_graphics.h` and links the installed `mln_test_graphics`.                                                                  |
| Python         | In use  | `tests/graphics.py` opens the library with `ctypes.CDLL` from the install's `lib` or `bin`, with a `ctypes.Structure` for the context.                     |
| .NET           | Planned | `[LibraryImport("mln_test_graphics")]`, resolved through the same `NativeLibrary` import resolver as the C API.                                            |
| Dart           | Planned | `DynamicLibrary.open` on the library path, with `Struct` classes for the info structs.                                                                     |
| Kotlin/JVM     | Planned | FFM `SymbolLookup.libraryLookup` on the library path.                                                                                                      |

A binding that runs its tests on a device, such as through the Android or
OpenHarmony emulator runners, pushes the library beside its test executables, as
the C suite's runners push the test plugin.

Go is the reference: `bindings/go/internal/testsupport/graphics.go` wraps the
interface, and the render tests attach through it on Metal, Vulkan, and EGL,
including in the Android and OpenHarmony emulators. The Go test task selects the
backend with a build tag named after the preset's, so a WGL build, which has no
backend tag, links no test graphics.
