#include <atomic>

#include "platform/android/java_vm.hpp"

// The JNI component headers require the umbrella's declaration order.
// clang-format off
#include <jni/jni.hpp> // IWYU pragma: keep
// clang-format on

#include <jni/functions.hpp>
#include <jni/ownership.hpp>

namespace mln::platform::android {
namespace {

auto vm_instance() -> std::atomic<JavaVM*>& {
  static std::atomic<JavaVM*> instance{nullptr};
  return instance;
}

auto thread_environment() -> jni::UniqueEnv& {
  thread_local jni::UniqueEnv instance;
  return instance;
}

}  // namespace

void initialize_java_vm(JNIEnv& env) {
  vm_instance().store(&jni::GetJavaVM(env), std::memory_order_release);
}

auto java_vm() -> JavaVM* {
  return vm_instance().load(std::memory_order_acquire);
}

void attach_thread() {
  auto* jvm = java_vm();
  auto& environment = thread_environment();
  if (jvm != nullptr && !environment) {
    environment = jni::GetAttachedEnv(*jvm);
  }
}

void detach_thread() { thread_environment().reset(); }

}  // namespace mln::platform::android
