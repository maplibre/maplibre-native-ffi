/**
 * @file maplibre_native_c/platform/win32.h
 * Present the map to a Win32 window, or render it into DXGI shared textures.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_WIN32_H
#define MAPLIBRE_NATIVE_C_PLATFORM_WIN32_H

#include <stdint.h>

#include "../base.h"
#include "../completion.h"
#include "../map.h"
#include "../render_session.h"
#include "../render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A Win32 window to present to, for mln_map_attach_win32_window(). */
typedef struct mln_win32_window_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `HWND`. */
  void* window MLN_BINDING("required=true");
} mln_win32_window_descriptor;

/** A new Win32 window for mln_render_session_set_win32_window_target(). */
typedef struct mln_win32_window_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `HWND`. */
  void* window MLN_BINDING("required=true");
} mln_win32_window_replacement;

/**
 * Creates a session that presents the map to a Win32 window, with a graphics
 * context that the library creates.
 *
 * A Vulkan build presents with `device`, or, when `device` is null, with a
 * device that the library creates. An OpenGL build uses a WGL context, and
 * `device` must be null. Attach with MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * An OpenGL build sets the window's pixel format, so don't set one yourself.
 *
 * The session presents without waiting for the display, so demand at most one
 * frame per display refresh. While the window is minimized, frame demands
 * finish with MLN_RENDER_RESULT_TARGET_NOT_READY.
 *
 * Keep the window until a release or replacement of the target, or a detach of
 * the session, completes.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `window` isn't a window, or, in an OpenGL
 *   build, already has another pixel format.
 * - MLN_STATUS_INVALID_STATE when `window` is minimized or has no client area.
 * - MLN_STATUS_UNSUPPORTED when you created `device` without
 *   mln_vulkan_device_options.presentation.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when `window` was minimized before the attach ran.
 * - MLN_STATUS_UNSUPPORTED when the GPU or the Vulkan loader can't present.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create the graphics context
 *   or present to the window.
 */
MLN_BINDING("execution=lifecycle;platforms=windows")
MLN_API mln_status mln_map_attach_win32_window(
  mln_map map, const mln_win32_window_descriptor* descriptor,
  mln_vulkan_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Win32 window that the session presents to.
 *
 * Use it to give the session a window again after
 * mln_render_session_release_target(), to move it to another window, or to
 * resize it. Once the replacement completes, the session no longer uses the
 * old window.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `window` isn't a window, or, in an OpenGL
 *   build, already has another pixel format.
 * - MLN_STATUS_INVALID_STATE when `window` is minimized or has no client area.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when `window` was minimized before the replacement
 *   ran.
 * - MLN_STATUS_UNSUPPORTED when, in a Vulkan build, the new window needs a
 *   different color format than the old one. Attach a new session instead.
 * - MLN_STATUS_NATIVE_ERROR when the library can't present to the new window.
 */
MLN_BINDING("execution=operation;platforms=windows")
MLN_API mln_status mln_render_session_set_win32_window_target(
  mln_render_session session, const mln_win32_window_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * A ring of DXGI shared textures that the library allocates, for
 * mln_map_attach_dxgi_shared_texture_ring().
 *
 * Each texture is a D3D11 2D texture with the map's size in pixels and the
 * format DXGI_FORMAT_B8G8R8A8_UNORM. It binds as a shader resource and a render
 * target, and is shared through an NT handle.
 */
typedef struct mln_dxgi_shared_texture_ring_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** How many textures the ring has, from 1 to 8. */
  uint32_t texture_count MLN_BINDING("required=true");
} mln_dxgi_shared_texture_ring_descriptor;

/** The DXGI shared texture of an acquired frame. */
typedef struct mln_dxgi_shared_texture_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The texture's index in the ring. */
  uint32_t slot;
  /** The texture's `DXGI_FORMAT`. */
  uint32_t format;
  /** The texture's shared NT `HANDLE`. */
  void* handle MLN_BINDING("lifetime=owner");
} mln_dxgi_shared_texture_frame;

/**
 * Creates a session that renders the map into a ring of DXGI shared textures
 * that the library allocates.
 *
 * Acquire each frame with mln_render_session_acquire_frame(), and get its
 * texture with mln_acquired_frame_get_dxgi_shared_texture().
 * mln_render_session_resize() gives the ring new textures of the new size.
 *
 * The session renders with the Vulkan device `device`, or, when `device` is
 * null, with one that the library creates. The textures belong to the
 * device's GPU, so open them on a D3D11 or D3D12 device of the same adapter. To
 * pick the GPU, create `device` with mln_vulkan_device_options.device_luid set
 * to your adapter's LUID. Attach with MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * For a reader that waits on no fence, such as Flutter's texture registrar,
 * attach without MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE in
 * mln_render_session_attach_options.producer_sync_kinds, so that each frame
 * finishes before you acquire it. Release a frame with
 * MLN_GPU_SYNC_CPU_COMPLETE only once the reader has finished with its texture.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED in an OpenGL build, or when the GPU can't share the
 *   textures: D3D11 has no adapter with the Vulkan device's LUID or can't
 *   share textures through NT handles, or the device lacks
 *   VK_KHR_external_memory_win32 or can't import D3D11 textures.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create its Vulkan device or
 *   a D3D11 device on the adapter.
 */
MLN_BINDING("execution=lifecycle;platforms=windows")
MLN_API mln_status mln_map_attach_dxgi_shared_texture_ring(
  mln_map map, const mln_dxgi_shared_texture_ring_descriptor* descriptor,
  mln_vulkan_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the DXGI shared texture of an acquired frame.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the texture.
 * Once you release or discard the frame, the session may render into the
 * texture again. Open the handle with `ID3D11Device1::OpenSharedResource1` or
 * `ID3D12Device::OpenSharedHandle`. A texture that you open keeps its memory,
 * but not its contents.
 *
 * The session owns the handle, so don't close it. To keep the handle after you
 * release the frame, duplicate it with `DuplicateHandle`.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The handle in `out_frame` stays valid until you release or discard the
 * frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps it valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=windows")
MLN_API mln_status mln_acquired_frame_get_dxgi_shared_texture(
  mln_acquired_frame frame,
  mln_dxgi_shared_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_WIN32_H
