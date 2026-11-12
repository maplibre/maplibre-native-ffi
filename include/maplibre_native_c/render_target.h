/**
 * @file maplibre_native_c/render_target.h
 * The targets that render sessions render into, and the options,
 * capabilities, graphics contexts, and GPU synchronization that they share.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_TARGET_H
#define MAPLIBRE_NATIVE_C_RENDER_TARGET_H

#include <stdint.h>

#include "base.h"
#include "map.h"
#include "wake.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A Vulkan non-dispatchable handle, such as a `VkImage`, as an integer. */
typedef uint64_t mln_vulkan_non_dispatchable_handle
  MLN_BINDING("native_object=true");

/** The null Vulkan non-dispatchable handle. */
#define MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL UINT64_C(0)

/** The most textures that a render session's texture ring can have. */
#define MLN_TEXTURE_RING_MAX_DEPTH 8u

/** The thread that runs a render session's graphics work. */
typedef enum mln_render_driver_kind : uint32_t {
  /** A library thread. */
  MLN_RENDER_DRIVER_CORE_WORKER = 1U,
  /** Your thread, when it calls mln_render_session_service_driver_work(). */
  MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD = 2U,
} mln_render_driver_kind;

/** Something that a render session can do. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_render_session_capability_flag : uint32_t {
  /** mln_render_session_acquire_frame() can take the frames it renders. */
  MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION = 1U << 0U,
  /** mln_render_session_read_texture() can copy its texture to memory. */
  MLN_RENDER_SESSION_CAPABILITY_READBACK = 1U << 1U,
  /** It shows frames on a surface. */
  MLN_RENDER_SESSION_CAPABILITY_PRESENTATION = 1U << 2U,
} mln_render_session_capability_flag;

/** A set of mln_render_session_capability_flag bits. */
typedef uint32_t mln_render_session_capability_flags;

/**
 * Locks or unlocks the Vulkan queue that you share with a render session.
 *
 * Threads:
 * It can run on a library thread, and it can't call the library.
 */
MLN_BINDING("synchronous=true")
typedef void (*mln_queue_lock_callback)(void* user_data);

/**
 * Your lock on a Vulkan queue that you and a render session both submit to.
 *
 * The session holds the lock only while it submits or presents to the queue.
 * Set both `lock` and `unlock`, or neither when you don't submit to the queue
 * yourself.
 */
typedef struct mln_queue_lock {
  mln_queue_lock_callback lock MLN_BINDING("nullable=true");
  mln_queue_lock_callback unlock MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
} mln_queue_lock MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * How one side waits for the other's GPU work on a shared texture.
 *
 * A sync file descriptor or an OpenGL fence signals once, so when
 * mln_acquired_frame_release() succeeds, the session takes the one you pass. A
 * Vulkan timeline semaphore, Metal shared event, or D3D12 fence counts up with
 * each frame, so the one you pass stays yours. Keep it valid until a detach or
 * release of the session completes.
 */
typedef enum mln_gpu_sync_kind : uint32_t {
  /** The work has already finished, so there is nothing to wait for. */
  MLN_GPU_SYNC_CPU_COMPLETE = 0U,
  /** `handle` is a `GLsync` in the session's share group. */
  MLN_GPU_SYNC_OPEN_GL_FENCE = 1U,
  /**
   * The work was submitted to the queue that both sides share, so work
   * submitted to that queue later runs after it.
   */
  MLN_GPU_SYNC_QUEUE_ORDER = 2U,
  /** `handle` is a `VkSemaphore`. Wait until it reaches `value`. */
  MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE = 3U,
  /** `handle` is a sync file descriptor. */
  MLN_GPU_SYNC_SYNC_FD = 4U,
  /** `handle` is an `MTLSharedEvent`. Wait until it reaches `value`. */
  MLN_GPU_SYNC_METAL_SHARED_EVENT = 5U,
  /**
   * `handle` is a shared Windows handle to an `ID3D12Fence`. Wait until it
   * reaches `value`.
   */
  MLN_GPU_SYNC_D3D12_FENCE = 6U,
} mln_gpu_sync_kind;

/** A GPU sync kind, as a bit in a set. */
typedef enum MLN_BINDING("kind=bitmask") mln_gpu_sync_kind_flag : uint32_t {
  MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE = 1U << 0U,
  MLN_GPU_SYNC_KIND_FLAG_OPEN_GL_FENCE = 1U << 1U,
  MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER = 1U << 2U,
  MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE = 1U << 3U,
  MLN_GPU_SYNC_KIND_FLAG_SYNC_FD = 1U << 4U,
  MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT = 1U << 5U,
  MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE = 1U << 6U,
} mln_gpu_sync_kind_flag;

/** A set of mln_gpu_sync_kind_flag bits. */
typedef uint32_t mln_gpu_sync_kind_flags;

/** Options for attaching a render session to a map. */
typedef struct mln_render_session_attach_options {
  uint32_t size;
  /**
   * The thread that runs the session's graphics work, which depends on the
   * target:
   *
   * - MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD for a WebGPU surface, or a
   *   borrowed ring of OpenGL or WebGPU textures.
   * - MLN_RENDER_DRIVER_CORE_WORKER for an offscreen target, a window, an HTML
   *   canvas, or a ring of Android hardware buffers, dmabufs, DXGI shared
   *   textures, or IOSurfaces.
   * - Either one for a Metal layer, or a borrowed ring of Metal or Vulkan
   *   textures.
   *
   * An attach with another driver fails with MLN_STATUS_UNSUPPORTED.
   */
  mln_render_driver_kind driver MLN_BINDING("required=true");
  /**
   * The kinds of mln_gpu_sync that you can wait on before you read an acquired
   * frame. The session uses one of them that its target supports. Zero lets the
   * session pick a kind that names no object to wait on.
   */
  mln_gpu_sync_kind_flags producer_sync_kinds;
  /**
   * Called when frame results are waiting for
   * mln_render_session_drain_frame_results(). It isn't called again until a
   * drain takes them.
   */
  mln_wake frame_wake;
  /**
   * Called when the session has something new to draw. Demand a frame at your
   * next display refresh. It isn't called again until you demand a frame, or
   * while the session has no target.
   */
  mln_wake redraw_wake;
  /**
   * Called when a session driven by MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
   * has work for mln_render_session_service_driver_work(). It isn't called
   * again until a service runs.
   */
  mln_wake driver_work_wake;
  /** Your lock on the Vulkan queue, when you submit to it yourself. */
  mln_queue_lock queue_lock;
} mln_render_session_attach_options;

/** What an attached render session supports. */
typedef struct mln_render_session_capabilities {
  uint32_t size;
  mln_render_driver_kind driver;
  /** How many textures the session's texture ring has. */
  uint32_t texture_ring_depth;
  mln_render_session_capability_flags flags;
  /**
   * The kind of mln_gpu_sync that mln_acquired_frame_get_producer_sync()
   * reports.
   */
  mln_gpu_sync_kind producer_sync_kind;
  /**
   * The kinds of mln_gpu_sync that mln_acquired_frame_release() accepts.
   * They depend on the target and on what the device supports.
   */
  mln_gpu_sync_kind_flags consumer_sync_kinds;
} mln_render_session_capabilities;

/** A point in GPU work that the other side waits for before using a texture. */
typedef struct mln_gpu_sync {
  uint32_t size;
  mln_gpu_sync_kind kind;
  /** The object to wait on, for the kinds that name one. */
  uint64_t handle MLN_BINDING("lifetime=owner;native_object=true");
  uint64_t value;
} mln_gpu_sync;

/** The Metal device that a render session uses. */
typedef struct mln_metal_context_descriptor {
  /** An `MTLDevice`. */
  void* device;
} mln_metal_context_descriptor;

/**
 * The Vulkan device and queue that a render session uses.
 *
 * Keep these objects alive until a detach or release of the session
 * completes, or an abandon of it returns.
 */
typedef struct mln_vulkan_context_descriptor {
  /** A `VkInstance`. */
  void* instance MLN_BINDING("required=true");
  /** A `VkPhysicalDevice`. */
  void* physical_device MLN_BINDING("required=true");
  /** A `VkDevice`. */
  void* device MLN_BINDING("required=true");
  /**
   * A `VkQueue` from `graphics_queue_family_index` that supports graphics. If
   * you submit to it too, set mln_render_session_attach_options.queue_lock.
   */
  void* graphics_queue MLN_BINDING("required=true");
  uint32_t graphics_queue_family_index;
  /** Your `vkGetInstanceProcAddr`, or null to use the Vulkan loader's. */
  void* get_instance_proc_addr;
  /** Your `vkGetDeviceProcAddr`, or null to look it up from the instance. */
  void* get_device_proc_addr;
} mln_vulkan_context_descriptor;

/** The WebGPU device that a render session uses. */
typedef struct mln_web_gpu_context_descriptor {
  /** A `WGPUInstance`, or null. */
  void* instance;
  /** A `WGPUDevice`. */
  void* device MLN_BINDING("required=true");
  /** A `WGPUQueue`, or null to use the device's queue. */
  void* queue;
} mln_web_gpu_context_descriptor;

/** An OpenGL context platform, as a bit in a set. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_open_gl_context_provider_flag : uint32_t {
  MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL = 1u << 0u,
  MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL = 1u << 1u,
  MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL = 1u << 2u,
} mln_open_gl_context_provider_flag;

/** A set of mln_open_gl_context_provider_flag bits. */
typedef uint32_t mln_open_gl_context_provider_flags;

/** The platform API that provides an OpenGL context. */
typedef enum mln_open_gl_context_platform : uint32_t {
  MLN_OPEN_GL_CONTEXT_PLATFORM_UNSPECIFIED = 0u,
  MLN_OPEN_GL_CONTEXT_PLATFORM_WGL = 1u,
  MLN_OPEN_GL_CONTEXT_PLATFORM_EGL = 2u,
  MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL = 3u,
} mln_open_gl_context_platform;

/** Whether a render session's OpenGL context shares objects with yours. */
typedef enum mln_open_gl_context_ownership : uint32_t {
  /** The session's context shares objects with your `share_context`. */
  MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED = 0u,
  /** The session's context shares objects with no other context. */
  MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED = 1u,
} mln_open_gl_context_ownership;

/** The OpenGL API that a context renders with. */
typedef enum mln_open_gl_client_api : uint32_t {
  MLN_OPEN_GL_CLIENT_API_UNSPECIFIED = 0u,
  MLN_OPEN_GL_CLIENT_API_GL = 1u,
  MLN_OPEN_GL_CLIENT_API_GLES = 2u,
} mln_open_gl_client_api;

/** The WGL objects that a render session makes its OpenGL context from. */
typedef struct mln_wgl_context_descriptor {
  /** An `HDC`. */
  void* device_context MLN_BINDING("required=true");
  /** Your `HGLRC`, for a shared context. */
  void* share_context;
  /** Your `wglGetProcAddress`, or null to use the system's. */
  void* get_proc_address;
} mln_wgl_context_descriptor;

/** The EGL objects that a render session makes its OpenGL context from. */
typedef struct mln_egl_context_descriptor {
  /** An `EGLDisplay`. */
  void* display MLN_BINDING("required=true");
  /** An `EGLConfig`. */
  void* config MLN_BINDING("required=true");
  /** Your `EGLContext`, for a shared context. */
  void* share_context;
  /** The API of a dedicated context. A shared context uses the API of yours. */
  mln_open_gl_client_api client_api;
  /** Your `eglGetProcAddress`, or null to use the system's. */
  void* get_proc_address;
} mln_egl_context_descriptor;

/** How a render session gets its WebGL context. */
typedef enum mln_web_gl_context_kind : uint32_t {
  /**
   * The session uses a context that you created with
   * `emscripten_webgl_create_context()`.
   */
  MLN_WEB_GL_CONTEXT_EXISTING = 0U,
  /**
   * The library transfers the canvas that `canvas_selector` names to a worker
   * and creates the context there.
   */
  MLN_WEB_GL_CONTEXT_TRANSFERRED_CANVAS = 1U,
} mln_web_gl_context_kind;

/** The WebGL context or canvas that a render session uses. */
typedef struct mln_web_gl_context_descriptor {
  mln_web_gl_context_kind kind;
  /** Your `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for an existing context. */
  int32_t context MLN_BINDING("native_object=true");
  /** The canvas's CSS selector, for a transferred canvas. */
  mln_buffer_view canvas_selector;
} mln_web_gl_context_descriptor;

/** The WGL, EGL, or WebGL part of an OpenGL context descriptor. */
typedef union mln_open_gl_context_descriptor_data {
  mln_wgl_context_descriptor wgl
    MLN_BINDING("variant=MLN_OPEN_GL_CONTEXT_PLATFORM_WGL");
  mln_egl_context_descriptor egl
    MLN_BINDING("variant=MLN_OPEN_GL_CONTEXT_PLATFORM_EGL");
  mln_web_gl_context_descriptor web_gl
    MLN_BINDING("variant=MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL");
} mln_open_gl_context_descriptor_data;

/**
 * How a render session gets the OpenGL context that it renders with.
 *
 * Keep the objects that it names alive until a detach or release of the
 * session completes, or an abandon of it returns.
 */
typedef struct mln_open_gl_context_descriptor {
  mln_open_gl_context_platform platform;
  mln_open_gl_context_ownership ownership;
  mln_open_gl_context_descriptor_data data MLN_BINDING("tag=platform");
} mln_open_gl_context_descriptor;

/** The kind of a render target, which says what its descriptor holds. */
typedef enum mln_render_target_kind : uint32_t {
  /** An image that the session allocates, and that you read back. */
  MLN_RENDER_TARGET_KIND_OFFSCREEN = 1u,
  /** A `CAMetalLayer`. */
  MLN_RENDER_TARGET_KIND_METAL_LAYER = 2u,
  /** A Win32 window. */
  MLN_RENDER_TARGET_KIND_WIN32_WINDOW = 3u,
  /** An X11 window, through Xlib. */
  MLN_RENDER_TARGET_KIND_XLIB_WINDOW = 4u,
  /** A Wayland surface. */
  MLN_RENDER_TARGET_KIND_WAYLAND_SURFACE = 5u,
  /** An Android `ANativeWindow`. */
  MLN_RENDER_TARGET_KIND_ANDROID_WINDOW = 6u,
  /** An OpenHarmony `OHNativeWindow`. */
  MLN_RENDER_TARGET_KIND_OHOS_WINDOW = 7u,
  /** A WebGPU surface on the device that you wrapped. */
  MLN_RENDER_TARGET_KIND_WEB_GPU_SURFACE = 8u,
  /** An HTML canvas in the page. */
  MLN_RENDER_TARGET_KIND_HTML_CANVAS = 9u,
} mln_render_target_kind;

/** A render target kind, as a bit in a set. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_render_target_kind_flag : uint32_t {
  MLN_RENDER_TARGET_KIND_FLAG_OFFSCREEN = 1u << 0u,
  MLN_RENDER_TARGET_KIND_FLAG_METAL_LAYER = 1u << 1u,
  MLN_RENDER_TARGET_KIND_FLAG_WIN32_WINDOW = 1u << 2u,
  MLN_RENDER_TARGET_KIND_FLAG_XLIB_WINDOW = 1u << 3u,
  MLN_RENDER_TARGET_KIND_FLAG_WAYLAND_SURFACE = 1u << 4u,
  MLN_RENDER_TARGET_KIND_FLAG_ANDROID_WINDOW = 1u << 5u,
  MLN_RENDER_TARGET_KIND_FLAG_OHOS_WINDOW = 1u << 6u,
  MLN_RENDER_TARGET_KIND_FLAG_WEB_GPU_SURFACE = 1u << 7u,
  MLN_RENDER_TARGET_KIND_FLAG_HTML_CANVAS = 1u << 8u,
} mln_render_target_kind_flag;

/** A set of mln_render_target_kind_flag bits. */
typedef uint32_t mln_render_target_kind_flags;

/**
 * An image that the session allocates, renders at the map's scale factor, and
 * reads back with mln_render_session_read_texture().
 */
typedef struct mln_offscreen_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
} mln_offscreen_target;

/**
 * A `CAMetalLayer` to present to.
 *
 * The session sets the layer's device, pixel format, and drawable size.
 */
typedef struct mln_metal_layer_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `CAMetalLayer*`. */
  void* layer MLN_BINDING("required=true");
} mln_metal_layer_target;

/**
 * A Win32 window to present to.
 *
 * On an OpenGL device, the session sets the window's pixel format, so don't
 * set one yourself. While the window is minimized, frame demands finish with
 * MLN_RENDER_RESULT_TARGET_NOT_READY.
 */
typedef struct mln_win32_window_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `HWND`. */
  void* window MLN_BINDING("required=true");
} mln_win32_window_target;

/**
 * An X11 window to present to, through Xlib.
 *
 * Keep the display connected until the session's detach completes.
 */
typedef struct mln_xlib_window_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `Display*`. */
  void* display MLN_BINDING("required=true");
  /** Your `Window`, on `display`. */
  uint64_t window MLN_BINDING("required=true;native_object=true");
} mln_xlib_window_target;

/**
 * A Wayland surface to present to.
 *
 * Keep the display connected until the session's detach completes. On an
 * OpenGL device, if you also use EGL on the display, initialize it before you
 * attach.
 */
typedef struct mln_wayland_surface_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `wl_display*`. */
  void* display MLN_BINDING("required=true");
  /** Your `wl_surface*`, on `display`. */
  void* surface MLN_BINDING("required=true");
} mln_wayland_surface_target;

/**
 * An Android window to present to.
 *
 * The session takes its own reference to the window, so you can release yours
 * once the call returns. When the platform destroys the window, such as in
 * `SurfaceHolder.Callback.surfaceDestroyed`, release the target with
 * mln_render_session_release_target(), or give the session another window
 * with mln_render_session_set_target(), without waiting for that to complete.
 * A frame to the destroyed window finishes with
 * MLN_RENDER_RESULT_TARGET_NOT_READY, or with MLN_RENDER_RESULT_RENDERED if
 * the window went away while the frame was presenting.
 */
typedef struct mln_android_window_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `ANativeWindow*`. */
  void* window MLN_BINDING("required=true");
} mln_android_window_target;

/**
 * An OpenHarmony window to present to.
 *
 * The session takes its own reference to the window, so you can release yours
 * once the call returns. When the platform destroys the window, such as in an
 * `XComponent`'s `onSurfaceDestroyed` callback, release the target or give
 * the session another window without waiting, as for an Android window.
 */
typedef struct mln_ohos_window_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `OHNativeWindow*`. */
  void* window MLN_BINDING("required=true");
} mln_ohos_window_target;

/**
 * A WebGPU surface to present to, which the session configures.
 *
 * Make it with the instance of the device that mln_gpu_device_wrap_web_gpu()
 * wrapped, and attach on that device.
 */
typedef struct mln_web_gpu_surface_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `WGPUSurface`. */
  void* surface MLN_BINDING("required=true");
  /** The `WGPUTextureFormat` to configure the surface with. */
  uint32_t format MLN_BINDING("required=true");
} mln_web_gpu_surface_target;

/**
 * An HTML canvas to present to.
 *
 * The library transfers the canvas to a worker, and makes its context there
 * with the device's backend.
 */
typedef struct mln_html_canvas_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** The canvas's CSS selector, such as `#map`. */
  mln_buffer_view selector MLN_BINDING("required=true");
} mln_html_canvas_target;

/** The descriptor of each kind of render target. */
typedef union mln_render_target_data {
  mln_offscreen_target offscreen
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_OFFSCREEN");
  mln_metal_layer_target metal_layer MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_METAL_LAYER;"
    "platforms=ios,macos,tvos,visionos"
  );
  mln_win32_window_target win32_window MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_WIN32_WINDOW;platforms=windows"
  );
  mln_xlib_window_target xlib_window MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_XLIB_WINDOW;platforms=freebsd,linux"
  );
  mln_wayland_surface_target wayland_surface MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_WAYLAND_SURFACE;platforms=freebsd,linux"
  );
  mln_android_window_target android_window MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_ANDROID_WINDOW;platforms=android"
  );
  mln_ohos_window_target ohos_window
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_OHOS_WINDOW;platforms=ohos");
  mln_web_gpu_surface_target web_gpu_surface
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_WEB_GPU_SURFACE;platforms=web");
  mln_html_canvas_target html_canvas
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_HTML_CANVAS;platforms=web");
} mln_render_target_data;

/**
 * What a render session renders into: one kind of target, and its
 * descriptor.
 *
 * Every target has a size in logical pixels, which sets the map's size. The
 * session renders it at the map's scale factor.
 *
 * The session uses the objects that the descriptor names until a replacement
 * or release of the target completes, a detach or release of the session
 * completes, or an abandon of it returns, except as a kind's descriptor says.
 * A window target presents without waiting for the display, so demand at most
 * one frame per display refresh.
 */
typedef struct mln_render_target {
  uint32_t size;
  mln_render_target_kind kind;
  mln_render_target_data data MLN_BINDING("tag=kind");
} mln_render_target;

/**
 * Computes the size in pixels of a target with a logical size and scale factor.
 */
MLN_API mln_status mln_logical_extent_physical_size(
  mln_logical_extent extent, uint32_t* out_width MLN_BINDING("direction=out"),
  uint32_t* out_height MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Reports the OpenGL context platforms that this build of the library has. */
MLN_API mln_open_gl_context_provider_flags
mln_open_gl_supported_context_provider_mask(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_TARGET_H
