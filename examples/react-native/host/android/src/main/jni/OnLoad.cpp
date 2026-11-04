#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>

#include <EGL/egl.h>
#include <ReactCommon/BindingsInstallerHolder.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <fbjni/fbjni.h>
#include <maplibre_native_c.h>

#include "Host.h"
#include "Surfaces.h"

namespace jni = facebook::jni;
namespace jsi = facebook::jsi;
using facebook::react::BindingsInstallerHolder;
using facebook::react::CallInvoker;

namespace {

// The display and config that every window surface and every session's
// dedicated OpenGL ES 3 context share. The process keeps them.
struct Egl {
  EGLDisplay display = EGL_NO_DISPLAY;
  EGLConfig config = nullptr;
  std::string error;
};

const Egl& egl() {
  static const Egl egl = [] {
    Egl result;
    result.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (
      result.display == EGL_NO_DISPLAY ||
      !eglInitialize(result.display, nullptr, nullptr)
    ) {
      result.error = "could not initialize the EGL display";
      return result;
    }
    const EGLint attributes[] = {
      EGL_SURFACE_TYPE,
      EGL_WINDOW_BIT,
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES3_BIT,
      EGL_RED_SIZE,
      8,
      EGL_GREEN_SIZE,
      8,
      EGL_BLUE_SIZE,
      8,
      EGL_ALPHA_SIZE,
      8,
      EGL_DEPTH_SIZE,
      24,
      EGL_STENCIL_SIZE,
      8,
      EGL_NONE,
    };
    EGLint count = 0;
    if (
      !eglChooseConfig(result.display, attributes, &result.config, 1, &count) ||
      count == 0
    ) {
      result.error = "no EGL config supports OpenGL ES 3 windows";
    }
    return result;
  }();
  return egl;
}

jsi::Value pointer(jsi::Runtime& runtime, const void* value) {
  return jsi::BigInt::fromUint64(
    runtime, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(value))
  );
}

// A SurfaceView's window, and the EGL window surface that the session presents
// to. The surface is made on the JavaScript thread when JavaScript first
// acquires the window.
class EglWindowSurface final : public mln::example::Surface {
 public:
  explicit EglWindowSurface(ANativeWindow* window) : window_(window) {}

  ~EglWindowSurface() override {
    if (surface_ != EGL_NO_SURFACE) eglDestroySurface(egl().display, surface_);
    ANativeWindow_release(window_);
  }

  jsi::Object describe(jsi::Runtime& runtime) override {
    const auto& egl = ::egl();
    if (!egl.error.empty()) throw jsi::JSError(runtime, egl.error);
    if (surface_ == EGL_NO_SURFACE) {
      EGLint format = 0;
      eglGetConfigAttrib(
        egl.display, egl.config, EGL_NATIVE_VISUAL_ID, &format
      );
      ANativeWindow_setBuffersGeometry(window_, 0, 0, format);
      surface_ =
        eglCreateWindowSurface(egl.display, egl.config, window_, nullptr);
      if (surface_ == EGL_NO_SURFACE) {
        char message[64];
        std::snprintf(
          message, sizeof message,
          "could not create an EGL window surface (0x%x)", eglGetError()
        );
        throw jsi::JSError(runtime, message);
      }
    }
    auto handles = jsi::Object(runtime);
    handles.setProperty(runtime, "kind", "egl");
    handles.setProperty(runtime, "display", pointer(runtime, egl.display));
    handles.setProperty(runtime, "config", pointer(runtime, egl.config));
    handles.setProperty(runtime, "surface", pointer(runtime, surface_));
    return handles;
  }

 private:
  ANativeWindow* window_;
  EGLSurface surface_ = EGL_NO_SURFACE;
};

}  // namespace

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  return jni::initialize(vm, [] {});
}

extern "C" JNIEXPORT jobject JNICALL
Java_org_maplibre_nativeffi_example_host_MaplibreHostModule_nativeBindingsInstaller(
  JNIEnv* env, jclass, jobject context
) {
  try {
    // The library needs the Java VM and the app before the first runtime.
    static std::once_flag initialized;
    static std::string failure;
    std::call_once(initialized, [&] {
      mln_diagnostic diagnostic = {.size = sizeof diagnostic};
      if (mln_android_init(env, context, &diagnostic) != MLN_STATUS_OK) {
        failure = diagnostic.message;
      }
    });
    if (!failure.empty()) {
      jni::throwNewJavaException(
        "java/lang/IllegalStateException", failure.c_str()
      );
    }
    return BindingsInstallerHolder::newObjectCxxArgs(
             [](
               jsi::Runtime& runtime,
               const std::shared_ptr<CallInvoker>& invoker
             ) { mln::example::install(runtime, invoker); }
    ).release();
  } catch (...) {
    jni::translatePendingCppExceptionToJavaException();
    return nullptr;
  }
}

extern "C" JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_example_host_MaplibreHostModule_nativeCleanup(
  JNIEnv*, jclass
) {
  mln::example::cleanup();
}

extern "C" JNIEXPORT jint JNICALL
Java_org_maplibre_nativeffi_example_host_MapSurfaceView_nativeAdd(
  JNIEnv* env, jclass, jobject surface
) {
  auto* window = ANativeWindow_fromSurface(env, surface);
  if (window == nullptr) return 0;
  return mln::example::surfaces::add(
    std::make_shared<EglWindowSurface>(window)
  );
}

extern "C" JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_example_host_MapSurfaceView_nativeWithdraw(
  JNIEnv*, jclass, jint surface
) {
  mln::example::surfaces::withdraw(surface);
}
