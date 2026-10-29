#include <ReactCommon/BindingsInstallerHolder.h>
#include <fbjni/fbjni.h>

#include "maplibre_native_c/android.h"
#include "module.h"

using namespace facebook;

struct ApplicationContext : jni::JavaClass<ApplicationContext> {
  static constexpr auto kJavaDescriptor =
    "Lcom/facebook/react/bridge/ReactApplicationContext;";
};

class MaplibreModule : public jni::JavaClass<MaplibreModule> {
 public:
  static constexpr auto kJavaDescriptor =
    "Lorg/maplibre/nativeffi/MaplibreNativeFfiModule;";

  static jni::local_ref<react::BindingsInstallerHolder::javaobject> installer(
    jni::alias_ref<MaplibreModule> module
  ) {
    auto owner = jni::make_global(module);
    return react::BindingsInstallerHolder::newObjectCxxArgs(
      [owner = std::move(owner)](
        jsi::Runtime& runtime,
        const std::shared_ptr<react::CallInvoker>& invoker
      ) mutable {
        auto getContext =
          owner->getClass()
            ->getMethod<jni::local_ref<ApplicationContext::javaobject>()>(
              "getReactApplicationContext"
            );
        auto context = getContext(owner);
        mln_diagnostic diagnostic = {.size = sizeof diagnostic};
        if (
          mln_android_init(
            jni::Environment::current(), context.get(), &diagnostic
          ) != MLN_STATUS_OK
        ) {
          throw jsi::JSError(runtime, diagnostic.message);
        }
        auto field = owner->getClass()->getField<jlong>("nativeModule");
        owner->setFieldValue(
          field, reinterpret_cast<jlong>(
                   new mln::react_native::Module(runtime, invoker)
                 )
        );
      }
    );
  }

  static void cleanup(jni::alias_ref<MaplibreModule>, jlong module) {
    if (module) reinterpret_cast<mln::react_native::Module*>(module)->cleanup();
  }
  static void destroy(jni::alias_ref<MaplibreModule>, jlong module) {
    delete reinterpret_cast<mln::react_native::Module*>(module);
  }
  static void registerNatives() {
    javaClassStatic()->registerNatives({
      makeNativeMethod("getBindingsInstaller", installer),
      makeNativeMethod("cleanup", cleanup),
      makeNativeMethod("destroy", destroy),
    });
  }
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  return jni::initialize(vm, [] { MaplibreModule::registerNatives(); });
}
