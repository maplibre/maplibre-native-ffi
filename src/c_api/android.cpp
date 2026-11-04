#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

#ifdef __ANDROID__
#include "platform/android/asset_manager.hpp"
#include "platform/android/frame_pacer.hpp"
#include "platform/android/i18n.hpp"
#include "platform/android/java_vm.hpp"
#endif

auto mln_android_init(
  void* jni_env, void* context, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    if (jni_env == nullptr || context == nullptr) {
      mln::core::set_thread_error("jni_env and context must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }

#ifndef __ANDROID__
    mln::core::set_thread_error("Android initialization is not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    auto& env = *static_cast<JNIEnv*>(jni_env);
    mln::platform::android::initialize_java_vm(env);
    mln::platform::android::initialize_i18n(env);

    const auto* asset_error =
      mln::platform::android_retain_asset_manager(jni_env, context);
    if (asset_error != nullptr) {
      mln::core::set_thread_error(asset_error);
      return MLN_STATUS_NATIVE_ERROR;
    }
    return MLN_STATUS_OK;
#endif
  });
}

auto mln_android_frame_pacer_create(
  const mln_frame_pacer_options* options, mln_frame_pacer* out_pacer,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef __ANDROID__
    static_cast<void>(options);
    static_cast<void>(out_pacer);
    mln::core::set_thread_error("Android frame pacers are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::platform::android::create_frame_pacer(options, out_pacer);
#endif
  });
}
