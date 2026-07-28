#if defined(MLN_FFI_OPENGL_PROVIDER_EGL) && defined(__linux__)

// The EGL entry points the library and its vendored sources call, defined here
// so nothing links against an EGL loader. The loader is opened on first use
// instead, which keeps the shipped binaries free of a build-host EGL dependency
// and lets a static consumer link without one. Vulkan resolves its loader the
// same way; see src/render/vulkan/vulkan_dispatch.hpp.
//
// These carry the names the EGL headers declare, so callers bind to them
// without knowing they are stubs.

#include <initializer_list>

#include <EGL/egl.h>
#include <dlfcn.h>

namespace {

auto egl_library() -> void* {
  static auto* handle = [] {
    for (const auto* name : {"libEGL.so.1", "libEGL.so"}) {
      if (
        auto* opened = dlopen(name, RTLD_LAZY | RTLD_LOCAL); opened != nullptr
      ) {
        return opened;
      }
    }
    return static_cast<void*>(nullptr);
  }();
  return handle;
}

template <typename Signature>
auto egl_proc(const char* name) -> Signature {
  auto* handle = egl_library();
  if (handle == nullptr) {
    return nullptr;
  }
  return reinterpret_cast<Signature>(dlsym(handle, name));
}

}  // namespace

// Each entry resolves once and forwards. A missing loader reports the same
// failure the real call would for an unusable display.
// clang-format off
extern "C" {

EGLBoolean eglBindAPI(EGLenum api) {
  static const auto fn = egl_proc<EGLBoolean (*)(EGLenum)>("eglBindAPI");
  return fn == nullptr ? EGL_FALSE : fn(api);
}

EGLBoolean eglChooseConfig(
  EGLDisplay display, const EGLint* attributes, EGLConfig* configs,
  EGLint config_size, EGLint* config_count
) {
  static const auto fn =
    egl_proc<EGLBoolean (*)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*)>(
      "eglChooseConfig");
  return fn == nullptr
           ? EGL_FALSE
           : fn(display, attributes, configs, config_size, config_count);
}

EGLContext eglCreateContext(
  EGLDisplay display, EGLConfig config, EGLContext share, const EGLint* attributes
) {
  static const auto fn =
    egl_proc<EGLContext (*)(EGLDisplay, EGLConfig, EGLContext, const EGLint*)>(
      "eglCreateContext");
  return fn == nullptr ? EGL_NO_CONTEXT : fn(display, config, share, attributes);
}

EGLSurface eglCreatePbufferSurface(
  EGLDisplay display, EGLConfig config, const EGLint* attributes
) {
  static const auto fn =
    egl_proc<EGLSurface (*)(EGLDisplay, EGLConfig, const EGLint*)>(
      "eglCreatePbufferSurface");
  return fn == nullptr ? EGL_NO_SURFACE : fn(display, config, attributes);
}

EGLBoolean eglDestroyContext(EGLDisplay display, EGLContext context) {
  static const auto fn =
    egl_proc<EGLBoolean (*)(EGLDisplay, EGLContext)>("eglDestroyContext");
  return fn == nullptr ? EGL_FALSE : fn(display, context);
}

EGLBoolean eglDestroySurface(EGLDisplay display, EGLSurface surface) {
  static const auto fn =
    egl_proc<EGLBoolean (*)(EGLDisplay, EGLSurface)>("eglDestroySurface");
  return fn == nullptr ? EGL_FALSE : fn(display, surface);
}

EGLContext eglGetCurrentContext() {
  static const auto fn = egl_proc<EGLContext (*)()>("eglGetCurrentContext");
  return fn == nullptr ? EGL_NO_CONTEXT : fn();
}

EGLDisplay eglGetCurrentDisplay() {
  static const auto fn = egl_proc<EGLDisplay (*)()>("eglGetCurrentDisplay");
  return fn == nullptr ? EGL_NO_DISPLAY : fn();
}

EGLSurface eglGetCurrentSurface(EGLint read_draw) {
  static const auto fn =
    egl_proc<EGLSurface (*)(EGLint)>("eglGetCurrentSurface");
  return fn == nullptr ? EGL_NO_SURFACE : fn(read_draw);
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType display) {
  static const auto fn =
    egl_proc<EGLDisplay (*)(EGLNativeDisplayType)>("eglGetDisplay");
  return fn == nullptr ? EGL_NO_DISPLAY : fn(display);
}

EGLint eglGetError() {
  static const auto fn = egl_proc<EGLint (*)()>("eglGetError");
  return fn == nullptr ? EGL_NOT_INITIALIZED : fn();
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char* name) {
  static const auto fn =
    egl_proc<__eglMustCastToProperFunctionPointerType (*)(const char*)>(
      "eglGetProcAddress");
  return fn == nullptr ? nullptr : fn(name);
}

EGLBoolean eglInitialize(EGLDisplay display, EGLint* major, EGLint* minor) {
  static const auto fn =
    egl_proc<EGLBoolean (*)(EGLDisplay, EGLint*, EGLint*)>("eglInitialize");
  return fn == nullptr ? EGL_FALSE : fn(display, major, minor);
}

EGLBoolean eglMakeCurrent(
  EGLDisplay display, EGLSurface draw, EGLSurface read, EGLContext context
) {
  static const auto fn =
    egl_proc<EGLBoolean (*)(EGLDisplay, EGLSurface, EGLSurface, EGLContext)>(
      "eglMakeCurrent");
  return fn == nullptr ? EGL_FALSE : fn(display, draw, read, context);
}

EGLenum eglQueryAPI() {
  static const auto fn = egl_proc<EGLenum (*)()>("eglQueryAPI");
  return fn == nullptr ? EGL_NONE : fn();
}

EGLBoolean eglQueryContext(
  EGLDisplay display, EGLContext context, EGLint attribute, EGLint* value
) {
  static const auto fn =
    egl_proc<EGLBoolean (*)(EGLDisplay, EGLContext, EGLint, EGLint*)>(
      "eglQueryContext");
  return fn == nullptr ? EGL_FALSE : fn(display, context, attribute, value);
}

EGLBoolean eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
  static const auto fn =
    egl_proc<EGLBoolean (*)(EGLDisplay, EGLSurface)>("eglSwapBuffers");
  return fn == nullptr ? EGL_FALSE : fn(display, surface);
}

EGLBoolean eglTerminate(EGLDisplay display) {
  static const auto fn = egl_proc<EGLBoolean (*)(EGLDisplay)>("eglTerminate");
  return fn == nullptr ? EGL_FALSE : fn(display);
}

}  // extern "C"
// clang-format on

#endif
