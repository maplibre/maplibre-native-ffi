# MapLibre Native FFI for ArkTS

`@maplibre/native-ffi-opengl` and `@maplibre/native-ffi-vulkan` are the
low-level ArkTS binding for the C API of
[MapLibre Native FFI](https://maplibre.org/maplibre-native-ffi/) on OpenHarmony,
with the OpenGL and the Vulkan backend. The C API embeds the
[MapLibre Native](https://github.com/maplibre/maplibre-native) map renderer.

Install one of them:

```bash
ohpm install @maplibre/native-ffi-opengl
```

```bash
ohpm install @maplibre/native-ffi-vulkan
```

An application's native module can use the same C library as the ArkTS binding.
After installing the OpenGL package, link it from the module's `CMakeLists.txt`
(adjust the path to your module's `oh_modules`):

```cmake
set(maplibre "${CMAKE_CURRENT_SOURCE_DIR}/../../../oh_modules/@maplibre/native-ffi-opengl")
target_include_directories(my_module PRIVATE "${maplibre}/include")
target_link_libraries(my_module PRIVATE
  "${maplibre}/libs/${OHOS_ARCH}/libmaplibre-native-c.so")
```

The headers are included with `#include <maplibre_native_c.h>`. Install only one
backend package in an application.

- [Install](https://maplibre.org/maplibre-native-ffi/install/#typescript)
- [Guides](https://maplibre.org/maplibre-native-ffi/guides/create-a-map/)
- [API reference](https://maplibre.org/maplibre-native-ffi/reference/typescript/)
