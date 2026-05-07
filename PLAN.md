# Support Multiple Build Variants

This guide shows contributors how to make each platform, architecture, and
render backend build as an isolated variant. Use it when adding a backend to an
existing platform or when adding a new platform family.

## Variant model

Use a variant key with this shape:

```text
<platform>-<arch>-<backend>
```

Examples:

- `macos-arm64-metal`
- `macos-arm64-vulkan`
- `linux-x64-vulkan`
- `linux-arm64-vulkan`

Each variant owns its CMake binary directory:

```text
build/<variant>
```

This keeps the CMake cache valid when switching between MapLibre Native render
backends. MapLibre Native supports one render backend per build, so each backend
needs a separate cache.

## Environment contract

Set these variables for every variant:

- `MLN_FFI_VARIANT`: full variant key, such as `macos-arm64-vulkan`
- `MLN_FFI_TARGET_TRIPLE`: platform and architecture, such as `macos-arm64`
- `MLN_FFI_RENDER_BACKEND`: render backend, such as `metal` or `vulkan`
- `MLN_FFI_BUILD_DIR`: CMake binary directory, usually `build/$MLN_FFI_VARIANT`

Build tools consume this contract instead of deriving the backend from the host
OS.

## Mise changes

1. Move CI-only tasks into the main `mise.toml`.
2. Use `MISE_ENV` for build variants.
3. Add variant environments for existing variants and the first new test
   variant:
   - `macos-arm64-metal`
   - `macos-arm64-vulkan`
   - `linux-x64-vulkan`
   - `linux-arm64-vulkan`
4. Point `DYLD_LIBRARY_PATH` and `LD_LIBRARY_PATH` at `$MLN_FFI_BUILD_DIR`.
5. Pass `$MLN_FFI_BUILD_DIR` to CMake, Zig, Swift, and artifact packaging.

## CMake changes

1. Add a cache variable named `MLN_FFI_RENDER_BACKEND`.
2. Validate supported backend values for the current platform.
3. Set exactly one MapLibre Native backend option:
   - `MLN_WITH_METAL=ON` for `metal`
   - `MLN_WITH_VULKAN=ON` for `vulkan`
   - later, `MLN_WITH_OPENGL=ON` for `opengl`
4. Select platform support by `CMAKE_SYSTEM_NAME`.
5. Select wrapper render-session sources by `MLN_FFI_RENDER_BACKEND`.
6. Keep Darwin platform support available to both macOS Metal and macOS Vulkan.

## Zig changes

1. Add `-Dcmake-artifact-dir` to tests and examples.
2. Add `-Drender-backend=<backend>` to tests and examples.
3. Use the selected backend in Zig code instead of `builtin.os.tag`.
4. Keep backend-specific include paths and system libraries behind the backend
   selection.
5. Use a variant-specific install prefix or cache path for examples when the
   default output directory would otherwise hide a stale link.

## Swift changes

1. Read `MLN_FFI_BUILD_DIR` in `Package.swift` and use it for `-L`.
2. Build with a variant-specific scratch path.
3. Keep the Swift map example on Metal until a Vulkan Swift view exists.

## CI changes

1. Replace target-only matrices with variant matrices.
2. Name native artifacts with the full variant key.
3. Package libraries from `$MLN_FFI_BUILD_DIR`.
4. Run examples only for variants they support.

## First validation target

Use macOS Vulkan to prove the refactor:

```sh
MISE_ENV=macos-arm64-vulkan mise run test
```

If MoltenVK is needed, add it through `pixi.toml` so the variant works through
the same dependency entrypoint as Linux Vulkan.
