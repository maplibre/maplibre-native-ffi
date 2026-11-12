/**
 * @file maplibre_native_c/platform/android.h
 * Set up the library in an Android app, render into hardware buffers, and
 * pace its frames to the display.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_ANDROID_H
#define MAPLIBRE_NATIVE_C_PLATFORM_ANDROID_H

#include <stdint.h>

#include "../base.h"
#include "../completion.h"
#include "../map.h"
#include "../render_session.h"
#include "../render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gives the library the Java VM and the app context that it needs on Android.
 *
 * Call it before creating a runtime. `jni_env` is the calling thread's
 * `JNIEnv*`, and `context` is an Android `Context`, such as the application.
 */
MLN_BINDING("platforms=android")
MLN_API mln_status mln_android_init(
  void* jni_env, void* context, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a frame pacer that follows the display through the calling thread's
 * `AChoreographer`.
 *
 * Call it on a thread with an `ALooper`, such as the app's main thread, and
 * keep that thread running its looper while sessions use the pacer.
 *
 * `AChoreographer` can stop while the phone's display is off, even while a
 * Surface on another display, such as an Android Auto projection, still shows
 * the map. The pacer then keeps demanding frames from a timer on the looper,
 * within the frame rate limit, until vsyncs arrive again.
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the calling thread has no `ALooper`.
 */
MLN_BINDING("platforms=android")
MLN_API mln_status mln_android_frame_pacer_create(
  const mln_frame_pacer_options* options MLN_BINDING("nullable=true"),
  mln_frame_pacer* out_pacer MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * A ring of Android hardware buffers that the library allocates, for
 * mln_map_attach_android_hardware_buffer_ring().
 *
 * Each buffer has the map's size in pixels, `logical_size` times the map's
 * scale factor, and the format AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM. Its usage
 * is AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
 * AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of `usage`.
 */
typedef struct mln_android_hardware_buffer_ring_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** How many buffers the ring has, from 1 to 8. */
  uint32_t buffer_count MLN_BINDING("required=true");
  /**
   * More `AHARDWAREBUFFER_USAGE_*` bits that your reads need, such as
   * AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
   */
  uint64_t usage;
} mln_android_hardware_buffer_ring_descriptor;

/** The Android hardware buffer of an acquired frame. */
typedef struct mln_android_hardware_buffer_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The buffer's index in the ring. */
  uint32_t slot;
  /** The `AHardwareBuffer*`. */
  void* buffer MLN_BINDING("lifetime=owner");
} mln_android_hardware_buffer_frame;

/**
 * Creates a session that renders the map into a ring of Android hardware
 * buffers that the library allocates.
 *
 * Acquire each frame with mln_render_session_acquire_frame(), and get its
 * buffer with mln_acquired_frame_get_android_hardware_buffer().
 * mln_render_session_resize() gives the ring new buffers of the new size.
 *
 * The session renders with `device`, a Vulkan or OpenGL device, or, when
 * `device` is null, with one that the library creates. An OpenGL device
 * renders with an EGL context on the default display. Attach with
 * MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * To show the buffers through Android's UI, such as with
 * `Bitmap.wrapHardwareBuffer`, attach without MLN_GPU_SYNC_KIND_FLAG_SYNC_FD
 * in mln_render_session_attach_options.producer_sync_kinds, so that each frame
 * finishes before you acquire it. Keep each frame acquired until the next one
 * replaces it on screen, then release it with MLN_GPU_SYNC_CPU_COMPLETE. Give
 * the ring 3 buffers, so the session can render while you hold two frames.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when Android is older than 8.0, API level 26, or
 *   when the device can't render into hardware buffers: the Vulkan
 *   device lacks VK_ANDROID_external_memory_android_hardware_buffer, or the EGL
 *   display lacks EGL_ANDROID_get_native_client_buffer or
 *   EGL_ANDROID_image_native_buffer.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create its Vulkan device or
 *   initialize the default EGL display.
 */
MLN_BINDING("execution=lifecycle;platforms=android")
MLN_API mln_status mln_map_attach_android_hardware_buffer_ring(
  mln_map map, const mln_android_hardware_buffer_ring_descriptor* descriptor,
  mln_gpu_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the Android hardware buffer of an acquired frame.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the buffer.
 * Once you release or discard the frame, the session may render into the
 * buffer again. A reference that you take with `AHardwareBuffer_acquire` keeps
 * the buffer's memory, but not its contents.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The buffer in `out_frame` stays valid until you release or discard the
 * frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps it valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=android")
MLN_API mln_status mln_acquired_frame_get_android_hardware_buffer(
  mln_acquired_frame frame,
  mln_android_hardware_buffer_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_ANDROID_H
