#include <ReactCommon/BindingsInstallerHolder.h>
#include <fbjni/fbjni.h>

#include "Probe.h"

using namespace facebook;

class LifecycleModule : public jni::JavaClass<LifecycleModule> {
 public:
  static constexpr auto kJavaDescriptor =
    "Lorg/maplibre/lifecycle/LifecycleProbeModule;";

  static jni::local_ref<react::BindingsInstallerHolder::javaobject> installer(
    jni::alias_ref<LifecycleModule> module
  ) {
    auto owner = jni::make_global(module);
    return react::BindingsInstallerHolder::newObjectCxxArgs(
      [owner = std::move(owner)](
        jsi::Runtime& runtime,
        const std::shared_ptr<react::CallInvoker>& invoker
      ) mutable {
        auto field = owner->getClass()->getField<jlong>("nativeProbe");
        owner->setFieldValue(
          field, reinterpret_cast<jlong>(new mln::test::Probe(runtime, invoker))
        );
      }
    );
  }

  static void cleanup(jni::alias_ref<LifecycleModule>, jlong probe) {
    reinterpret_cast<mln::test::Probe*>(probe)->cleanup();
  }

  static void destroy(jni::alias_ref<LifecycleModule>, jlong probe) {
    delete reinterpret_cast<mln::test::Probe*>(probe);
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
  return jni::initialize(vm, [] { LifecycleModule::registerNatives(); });
}
