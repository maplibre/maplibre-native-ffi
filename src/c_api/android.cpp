#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

#ifdef __ANDROID__
extern "C" auto mlnffi_rust_android_init_tls_verifier(
  void* jni_env, void* context
) -> char*;
extern "C" auto mlnffi_rust_android_error_free(char* error) -> void;
#endif

auto mln_android_init(void* jni_env, void* jni_class, void* context) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    // JavaCPP static native methods pass the declaring class between JNIEnv*
    // and the Java arguments. Keep the C ABI compatible with that call shape.
    (void)jni_class;
    if (jni_env == nullptr || context == nullptr) {
      mln::core::set_thread_error("jni_env and context must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }

#ifndef __ANDROID__
    mln::core::set_thread_error("Android initialization is not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    auto* error = mlnffi_rust_android_init_tls_verifier(jni_env, context);
    if (error == nullptr) {
      return MLN_STATUS_OK;
    }

    mln::core::set_thread_error(error);
    mlnffi_rust_android_error_free(error);
    return MLN_STATUS_NATIVE_ERROR;
#endif
  });
}
