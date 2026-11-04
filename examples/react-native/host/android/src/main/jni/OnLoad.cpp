#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

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

// A SurfaceView's window. The session takes its own reference when it attaches,
// and this one keeps the window valid until then.
class WindowSurface final : public mln::example::Surface {
 public:
  explicit WindowSurface(ANativeWindow* window) : window_(window) {}

  ~WindowSurface() override { ANativeWindow_release(window_); }

  jsi::Object describe(jsi::Runtime& runtime) override {
    auto handles = jsi::Object(runtime);
    handles.setProperty(runtime, "kind", "android");
    handles.setProperty(
      runtime, "window",
      jsi::BigInt::fromUint64(
        runtime, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(window_))
      )
    );
    return handles;
  }

 private:
  ANativeWindow* window_;
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
  return mln::example::surfaces::add(std::make_shared<WindowSurface>(window));
}

extern "C" JNIEXPORT void JNICALL
Java_org_maplibre_nativeffi_example_host_MapSurfaceView_nativeWithdraw(
  JNIEnv*, jclass, jint surface
) {
  mln::example::surfaces::withdraw(surface);
}
