#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/surface_session.hpp"
#include "render/texture_session.hpp"

#ifdef __ANDROID__
#include "platform/android/asset_manager.hpp"
#include "platform/android/frame_pacer.hpp"
#include "platform/android/i18n.hpp"
#include "platform/android/java_vm.hpp"
#include "render/android_hardware_buffer_session.hpp"
#include "render/android_surface_session.hpp"
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

auto mln_map_attach_android_surface(
  mln_map map, const mln_android_surface_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef __ANDROID__
    static_cast<void>(map);
    static_cast<void>(device);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_android_surface_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("Android surfaces are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::android_surface_attach_start(
      map, descriptor, device, options, out_session, completion
    );
#endif
  });
}

auto mln_render_session_set_android_surface_target(
  mln_render_session session,
  const mln_android_surface_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef __ANDROID__
    static_cast<void>(session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_android_surface_replacement(replacement);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("Android surfaces are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::android_surface_set_target_start(
      session, replacement, completion
    );
#endif
  });
}

auto mln_map_attach_android_hardware_buffer_ring(
  mln_map map, const mln_android_hardware_buffer_ring_descriptor* descriptor,
  mln_vulkan_device device, const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
#ifndef __ANDROID__
    static_cast<void>(map);
    static_cast<void>(device);
    static_cast<void>(options);
    static_cast<void>(out_session);
    static_cast<void>(completion);
    const auto status =
      mln::core::validate_android_hardware_buffer_ring_descriptor(descriptor);
    if (status != MLN_STATUS_OK) return status;
    mln::core::set_thread_error("Android hardware buffers are not supported");
    return MLN_STATUS_UNSUPPORTED;
#else
    return mln::core::android_hardware_buffer_attach_start(
      map, descriptor, device, options, out_session, completion
    );
#endif
  });
}

auto mln_acquired_frame_get_android_hardware_buffer(
  mln_acquired_frame frame, mln_android_hardware_buffer_frame* out_frame,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::acquired_frame_get_android_hardware_buffer(
      frame, out_frame
    );
  });
}
