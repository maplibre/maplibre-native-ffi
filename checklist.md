# ANGLE macOS OpenGL/EGL Checklist

Broad phases are grouped so each phase can land as a separate commit.

## Phase 1: Dependency Pinning and Fetching

- [x] Add `third_party/angle/manifest.json` with pinned Kivy ANGLE artifacts:
      `chromium-7151_rev1`, macOS arm64 URL, macOS universal URL, and SHA-256
      values.
- [x] Add `third_party/angle/.gitignore` so extracted ANGLE archives and
      libraries stay out of git while the manifest remains tracked.
- [x] Add a mise task `deps:angle` in `mise.toml` that downloads the platform
      artifact, verifies SHA-256, extracts to
      `third_party/angle/chromium-7151_rev1/macos-arm64`, and exits successfully
      when the verified extraction already exists.

## Phase 2: Build Variant and CMake Wiring

- [x] Add `.mise/config.macos-arm64-angle.toml` with
      `MLN_FFI_VARIANT=macos-arm64-angle`, `MLN_FFI_TARGET_TRIPLE=macos-arm64`,
      `MLN_FFI_RENDER_BACKEND=opengl`, `MLN_FFI_OPENGL_CONTEXT_PROVIDER=egl`,
      `MLN_FFI_ANGLE_ROOT={{config_root}}/third_party/angle/chromium-7151_rev1/macos-arm64`,
      and a dedicated build dir.
- [x] Update `mise.toml` `configure` so OpenGL builds pass
      `-DMLN_FFI_OPENGL_CONTEXT_PROVIDER="$MLN_FFI_OPENGL_CONTEXT_PROVIDER"` and
      ANGLE builds pass `-DMLN_FFI_ANGLE_ROOT="$MLN_FFI_ANGLE_ROOT"`.
- [x] Update `cmake/mln_options.cmake` to allow
      `MLN_FFI_OPENGL_CONTEXT_PROVIDER=egl` on macOS only when
      `MLN_FFI_ANGLE_ROOT` is set.
- [x] Add `cmake/angle.cmake` defining imported targets `ANGLE::EGL` and
      `ANGLE::GLESv2` from `MLN_FFI_ANGLE_ROOT`, including header dirs and dylib
      locations.
- [x] Update `cmake/render/opengl.cmake`: for macOS EGL, use ANGLE targets, add
      ANGLE include dirs, link `ANGLE::EGL` and `ANGLE::GLESv2`, and define
      `MLN_FFI_OPENGL_PROVIDER_EGL=1` plus `MLN_FFI_OPENGL_PROVIDER_ANGLE=1`.
- [x] Ensure build-tree runtime loading works by adding the ANGLE library
      directory to build RPATH or copying `libEGL.dylib` and `libGLESv2.dylib`
      next to `libmaplibre-native-c.dylib`.

## Phase 3: Shared EGL Provider Implementation

- [ ] Replace Linux-specific EGL guards in `src/render/opengl/*` with
      `MLN_FFI_OPENGL_PROVIDER_EGL`.
- [ ] Refactor duplicated EGL context code from texture and surface sessions
      into a shared project helper, such as
      `src/render/opengl/egl_context.hpp/.cpp`.
- [ ] Implement ANGLE display creation using `eglGetPlatformDisplay` or
      `eglGetPlatformDisplayEXT` with `EGL_PLATFORM_ANGLE_ANGLE`,
      `EGL_PLATFORM_ANGLE_TYPE_ANGLE`, `EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE`,
      `EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE`, and
      `EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE`.
- [ ] Add a small internal ANGLE/EGL bootstrap path for tests and examples that
      creates `EGLDisplay`, chooses an ES3 pbuffer-capable `EGLConfig`, creates
      a share `EGLContext`, and fills the existing `mln_egl_context_descriptor`.

## Phase 4: Documentation

- [ ] Update public docs and header wording that currently says EGL is
      Linux-only.
- [ ] Add `macos-arm64-angle` to docs as experimental development support once
      smoke tests pass.

## Phase 5: Verification

- [ ] Verify `mise -E macos-arm64-angle run deps:angle`.
- [ ] Verify `mise -E macos-arm64-angle run build`.
- [ ] Verify `mise -E macos-arm64-angle run test`.
- [ ] Verify a headless readback smoke test on macOS through ANGLE.
