/**
 * @file maplibre_native_c/render_target.h
 * The targets that render sessions render into, and the options,
 * capabilities, graphics contexts, and GPU synchronization that they share.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_TARGET_H
#define MAPLIBRE_NATIVE_C_RENDER_TARGET_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "map.h"
#include "wake.h"
#include "web.h"

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
 * Locks or unlocks the Vulkan queue that you share with the render sessions of
 * a GPU device.
 *
 * Threads:
 * It can run on a library thread, and it can't call the library.
 */
MLN_BINDING("synchronous=true")
typedef void (*mln_queue_lock_callback)(void* user_data);

/**
 * Your lock on a Vulkan queue that you and the render sessions of a GPU device
 * all submit to.
 *
 * A session holds the lock only while it submits or presents to the queue, or
 * submits the empty batch that drains its work from the queue. It never holds
 * the lock while it waits. Set both `lock` and `unlock`, or neither when you
 * don't submit to the queue yourself.
 *
 * Don't hold the lock while you call the library. A call such as
 * mln_render_session_abandon(), mln_render_session_release(),
 * mln_render_session_dispose(), or mln_render_session_service_driver_work()
 * can wait for a session's work, which takes the lock.
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
  /**
   * `handle` is a sync file descriptor. A descriptor of -1, as Vulkan exports
   * for a semaphore that has already signaled, counts as
   * MLN_GPU_SYNC_CPU_COMPLETE.
   */
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

/** A way to share a texture's memory with another API or process. */
typedef enum mln_gpu_export_kind : uint32_t {
  /** The texture isn't shared. */
  MLN_GPU_EXPORT_KIND_NONE = 0U,
  /** An `IOSurfaceRef`. */
  MLN_GPU_EXPORT_KIND_IO_SURFACE = 1U,
  /** A Linux dmabuf, as file descriptors with a DRM format and modifier. */
  MLN_GPU_EXPORT_KIND_DMA_BUF = 2U,
  /** An `AHardwareBuffer*`. */
  MLN_GPU_EXPORT_KIND_ANDROID_HARDWARE_BUFFER = 3U,
  /** A DXGI shared texture's NT handle. */
  MLN_GPU_EXPORT_KIND_DXGI_SHARED_HANDLE = 4U,
  /** A Vulkan or OpenGL opaque memory file descriptor. */
  MLN_GPU_EXPORT_KIND_OPAQUE_FD = 5U,
} mln_gpu_export_kind;

/** A way to share a texture's memory, as a bit in a set. */
typedef enum MLN_BINDING("kind=bitmask") mln_gpu_export_kind_flag : uint32_t {
  MLN_GPU_EXPORT_KIND_FLAG_IO_SURFACE = 1U << 0U,
  MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF = 1U << 1U,
  MLN_GPU_EXPORT_KIND_FLAG_ANDROID_HARDWARE_BUFFER = 1U << 2U,
  MLN_GPU_EXPORT_KIND_FLAG_DXGI_SHARED_HANDLE = 1U << 3U,
  MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD = 1U << 4U,
} mln_gpu_export_kind_flag;

/** A set of mln_gpu_export_kind_flag bits. */
typedef uint32_t mln_gpu_export_kind_flags;

/** A format of depth and stencil textures, as a bit in a set. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_depth_stencil_format_flag : uint32_t {
  /**
   * 24-bit normalized depth and 8-bit stencil: VK_FORMAT_D24_UNORM_S8_UINT,
   * MTLPixelFormatDepth24Unorm_Stencil8, GL_DEPTH24_STENCIL8, or
   * WGPUTextureFormat_Depth24PlusStencil8 (`depth24plus-stencil8`).
   */
  MLN_DEPTH_STENCIL_FORMAT_FLAG_DEPTH24_STENCIL8 = 1U << 0U,
  /**
   * 32-bit float depth and 8-bit stencil: VK_FORMAT_D32_SFLOAT_S8_UINT,
   * MTLPixelFormatDepth32Float_Stencil8, GL_DEPTH32F_STENCIL8, or
   * WGPUTextureFormat_Depth32FloatStencil8 (`depth32float-stencil8`).
   */
  MLN_DEPTH_STENCIL_FORMAT_FLAG_DEPTH32_FLOAT_STENCIL8 = 1U << 1U,
} mln_depth_stencil_format_flag;

/** A set of mln_depth_stencil_format_flag bits. */
typedef uint32_t mln_depth_stencil_format_flags;

/** Options for attaching a render session to a map. */
typedef struct mln_render_session_attach_options {
  uint32_t size;
  /**
   * The thread that runs the session's graphics work, which depends on the
   * target:
   *
   * - MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD for a borrowed ring of WebGPU
   *   textures.
   * - MLN_RENDER_DRIVER_CORE_WORKER for an offscreen target, a window, or an
   *   exported ring.
   * - Either one for a web canvas. In a browser both run on the thread that
   *   loaded the library: MLN_RENDER_DRIVER_CORE_WORKER in the library's own
   *   tasks, and MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD when you service the
   *   session.
   * - Either one for a Metal layer, a Vulkan surface, an OpenGL surface, or a
   *   borrowed ring of Metal, Vulkan, or OpenGL textures. A build may drive
   *   these on MLN_RENDER_DRIVER_CORE_WORKER only, and fails an attach with
   *   MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD then.
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
  /**
   * Whether an exported ring keeps the depth and stencil of each frame for
   * you, so that you can draw into the frame depth-tested against the map, as
   * mln_frame_projection says. The getter of the device's API, such as
   * mln_acquired_frame_get_vulkan_texture(), gets a frame's depth texture on
   * the device, and mln_acquired_frame_get_opaque_fd() shares it across APIs
   * when mln_gpu_device_capabilities.depth_export_kinds has the ring's kind.
   * Keeping depth costs memory bandwidth, most on tile-based GPUs. A borrowed
   * ring keeps depth in the textures that you give it instead, and the other
   * targets keep none, so an attach to them with `keep_depth` fails with
   * MLN_STATUS_UNSUPPORTED.
   */
  bool keep_depth;
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
  /**
   * How an exported ring shares its textures, which says which getter gets an
   * acquired frame's texture, or MLN_GPU_EXPORT_KIND_NONE for another target.
   */
  mln_gpu_export_kind export_kind;
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
   * you submit to it too, give the device your mln_queue_lock.
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
  /** Your `GPUDevice`. Sessions submit to its `queue`. */
  mln_web_object device MLN_BINDING("required=true");
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

/** The OpenGL API that a context renders with. */
typedef enum mln_open_gl_client_api : uint32_t {
  MLN_OPEN_GL_CLIENT_API_UNSPECIFIED = 0u,
  MLN_OPEN_GL_CLIENT_API_GL = 1u,
  MLN_OPEN_GL_CLIENT_API_GLES = 2u,
} mln_open_gl_client_api;

/** Your WGL context, which each session's context shares objects with. */
typedef struct mln_wgl_context_descriptor {
  /** An `HDC` with the pixel format of `share_context`. */
  void* device_context MLN_BINDING("required=true");
  /** Your `HGLRC`. */
  void* share_context MLN_BINDING("required=true");
  /** Your `wglGetProcAddress`, or null to use the system's. */
  void* get_proc_address;
} mln_wgl_context_descriptor;

/**
 * Your EGL context, which each session's context shares objects with.
 *
 * Each session makes its context on `display` with `config` and the client API
 * and version of `share_context`.
 */
typedef struct mln_egl_context_descriptor {
  /** An `EGLDisplay`, which you initialized. */
  void* display MLN_BINDING("required=true");
  /** The `EGLConfig` of `share_context`. */
  void* config MLN_BINDING("required=true");
  /** Your `EGLContext`. */
  void* share_context MLN_BINDING("required=true");
  /** Your `eglGetProcAddress`, or null to use the system's. */
  void* get_proc_address;
} mln_egl_context_descriptor;

/** The WebGL2 context that an OpenGL device wraps. */
typedef struct mln_web_gl_context_descriptor {
  /** Your `WebGL2RenderingContext`. */
  mln_web_object context MLN_BINDING("required=true");
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
 * Your OpenGL context, for mln_gpu_device_wrap_open_gl(): the root of the
 * share group that the device's sessions join.
 */
typedef struct mln_open_gl_context_descriptor {
  mln_open_gl_context_platform platform;
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
  /** A `VkSurfaceKHR` on the instance of a Vulkan device. */
  MLN_RENDER_TARGET_KIND_VULKAN_SURFACE = 10u,
  /** An `EGLSurface` or `HDC` for the context that an OpenGL device wraps. */
  MLN_RENDER_TARGET_KIND_OPEN_GL_SURFACE = 11u,
  /** A ring of your Metal textures. */
  MLN_RENDER_TARGET_KIND_METAL_BORROWED_TEXTURES = 12u,
  /** A ring of your Vulkan images. */
  MLN_RENDER_TARGET_KIND_VULKAN_BORROWED_TEXTURES = 13u,
  /** A ring of your OpenGL textures. */
  MLN_RENDER_TARGET_KIND_OPEN_GL_BORROWED_TEXTURES = 14u,
  /** A ring of your WebGPU textures. */
  MLN_RENDER_TARGET_KIND_WEB_GPU_BORROWED_TEXTURES = 15u,
  /** A ring of textures that the session allocates and shares with you. */
  MLN_RENDER_TARGET_KIND_EXPORTED_RING = 16u,
  /** A canvas to present to, with the device's backend. */
  MLN_RENDER_TARGET_KIND_WEB_CANVAS = 17u,
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
  MLN_RENDER_TARGET_KIND_FLAG_VULKAN_SURFACE = 1u << 9u,
  MLN_RENDER_TARGET_KIND_FLAG_OPEN_GL_SURFACE = 1u << 10u,
  MLN_RENDER_TARGET_KIND_FLAG_METAL_BORROWED_TEXTURES = 1u << 11u,
  MLN_RENDER_TARGET_KIND_FLAG_VULKAN_BORROWED_TEXTURES = 1u << 12u,
  MLN_RENDER_TARGET_KIND_FLAG_OPEN_GL_BORROWED_TEXTURES = 1u << 13u,
  MLN_RENDER_TARGET_KIND_FLAG_WEB_GPU_BORROWED_TEXTURES = 1u << 14u,
  MLN_RENDER_TARGET_KIND_FLAG_EXPORTED_RING = 1u << 15u,
  MLN_RENDER_TARGET_KIND_FLAG_WEB_CANVAS = 1u << 16u,
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
 * A canvas to present to.
 *
 * The session makes the canvas's "webgpu" or "webgl2" context with the
 * device's backend, and sets the canvas's size to `logical_size` at the map's
 * scale factor. Attached without a device, it renders with the build's
 * backend: a WebGPU build asks the browser for a device, and attaches once it
 * has one. For an OpenGL device that wraps your WebGL2 context, the canvas must
 * be the context's own.
 */
typedef struct mln_web_canvas_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /**
   * An `OffscreenCanvas`, such as one from
   * `HTMLCanvasElement.transferControlToOffscreen()` that you posted to the
   * library's Web Worker, or an `HTMLCanvasElement` when the library runs on
   * the page's thread.
   */
  mln_web_object canvas MLN_BINDING("required=true");
  /**
   * For a WebGPU device, the `GPUTextureFormat` to configure the canvas with,
   * such as `bgra8unorm`. Empty for a format that the device's adapter
   * prefers.
   */
  mln_buffer_view format;
} mln_web_canvas_target;

/**
 * Your Vulkan surface to present to.
 *
 * Make it with the instance of the Vulkan device that you attach on. The
 * device's queue family must support presenting to the surface. The session
 * creates the swapchain, and destroys it but not the surface.
 */
typedef struct mln_vulkan_surface_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** Your `VkSurfaceKHR`. */
  mln_vulkan_non_dispatchable_handle surface MLN_BINDING("required=true");
} mln_vulkan_surface_target;

/**
 * Your OpenGL surface to present to, with the context that
 * mln_gpu_device_wrap_open_gl() wrapped.
 *
 * The session makes its context current on the surface for each frame, and
 * swaps the surface's buffers to present.
 */
typedef struct mln_open_gl_surface_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /**
   * Your `EGLSurface` on the context's display, with its config, or your `HDC`
   * with the context's pixel format.
   */
  void* surface MLN_BINDING("required=true");
} mln_open_gl_surface_target;

/** A Metal texture of a borrowed ring. */
typedef struct mln_metal_borrowed_texture {
  /** Your `MTLTexture`. */
  void* texture MLN_BINDING("required=true");
} mln_metal_borrowed_texture;

/**
 * A ring of your Metal textures to render into, on the Metal device that you
 * attach on.
 *
 * `physical_width` and `physical_height` give the textures' size in pixels,
 * which is usually `logical_size` times the map's scale factor. The textures
 * must all have that size, the device's `MTLDevice`, one pixel format, one
 * sample, and `MTLTextureUsageRenderTarget`. The ring holds up to
 * MLN_TEXTURE_RING_MAX_DEPTH distinct textures.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as mln_frame_projection says: one for each of `textures`, at the
 * same index, or none. Each frame clears the one at its slot, renders the
 * map's depth and stencil into it, and stores them. They must have the size of
 * `textures`, one pixel format of
 * mln_gpu_device_capabilities.depth_stencil_formats, one sample,
 * `MTLTextureUsageRenderTarget`, and a storage mode other than
 * `MTLStorageModeMemoryless`.
 */
typedef struct mln_metal_borrowed_textures_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_metal_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  const mln_metal_borrowed_texture* depth_textures
    MLN_BINDING("length=depth_texture_count");
  size_t depth_texture_count;
} mln_metal_borrowed_textures_target;

/** A Vulkan image of a borrowed ring. */
typedef struct mln_vulkan_borrowed_texture {
  /** Your `VkImage`. */
  mln_vulkan_non_dispatchable_handle image MLN_BINDING("required=true");
  /** A 2D `VkImageView` of `image` with one mip level and one layer. */
  mln_vulkan_non_dispatchable_handle image_view MLN_BINDING("required=true");
} mln_vulkan_borrowed_texture;

/**
 * The queue family that a borrowed image goes to after each frame.
 *
 * With MLN_VULKAN_IMAGE_OWNER_EXTERNAL or MLN_VULKAN_IMAGE_OWNER_FOREIGN,
 * acquire the image on your side before you read it. You don't need to release
 * it back.
 */
typedef enum mln_vulkan_image_owner : uint32_t {
  /** The image stays with the queue family of the device's queue. */
  MLN_VULKAN_IMAGE_OWNER_GRAPHICS_QUEUE = 0U,
  /** Each frame releases the image to VK_QUEUE_FAMILY_EXTERNAL. */
  MLN_VULKAN_IMAGE_OWNER_EXTERNAL = 1U,
  /** Each frame releases the image to VK_QUEUE_FAMILY_FOREIGN_EXT. */
  MLN_VULKAN_IMAGE_OWNER_FOREIGN = 2U,
} mln_vulkan_image_owner;

/**
 * A ring of your Vulkan images to render into, on the Vulkan device that you
 * attach on.
 *
 * `physical_width` and `physical_height` give the images' size in pixels,
 * which is usually `logical_size` times the map's scale factor. Each image must
 * have that size, `format`, and `VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT`, and
 * belong to the device's `VkDevice`. The ring holds up to
 * MLN_TEXTURE_RING_MAX_DEPTH distinct images.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as mln_frame_projection says: one for each of `textures`, at the
 * same index, or none. Each frame clears the one at its slot, renders the
 * map's depth and stencil into it, stores them, and leaves it in
 * `depth_final_layout`, with `owner` as for the color image. They must have the
 * size of `textures`, `depth_format`, one sample, and
 * `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT`, and their views both the
 * depth and the stencil aspect.
 */
typedef struct mln_vulkan_borrowed_textures_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_vulkan_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  /** The images' `VkFormat`. */
  uint32_t format MLN_BINDING("required=true");
  /** The `VkImageLayout` that each frame leaves its image in. */
  uint32_t final_layout MLN_BINDING("required=true");
  mln_vulkan_image_owner owner;
  const mln_vulkan_borrowed_texture* depth_textures
    MLN_BINDING("length=depth_texture_count");
  size_t depth_texture_count;
  /**
   * The depth images' `VkFormat`, of
   * mln_gpu_device_capabilities.depth_stencil_formats.
   */
  uint32_t depth_format;
  /** The `VkImageLayout` that each frame leaves its depth image in. */
  uint32_t depth_final_layout;
} mln_vulkan_borrowed_textures_target;

/** An OpenGL texture of a borrowed ring. */
typedef struct mln_open_gl_borrowed_texture {
  /** Your texture name. */
  uint32_t texture MLN_BINDING("required=true;native_object=true");
} mln_open_gl_borrowed_texture;

/**
 * A ring of your OpenGL textures to render into, on an OpenGL device that
 * mln_gpu_device_wrap_open_gl() made from your context.
 *
 * `physical_width` and `physical_height` give the textures' size in pixels,
 * which is usually `logical_size` times the map's scale factor. Each texture
 * must be in your context's share group, with RGBA8 storage of that size. The
 * ring holds up to MLN_TEXTURE_RING_MAX_DEPTH distinct textures.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as mln_frame_projection says: one for each of `textures`, at the
 * same index, or none. Each frame clears the one at its slot, and renders the
 * map's depth and stencil into it. They must be `GL_TEXTURE_2D` textures in the
 * share group, with storage of the size of `textures` in a format of
 * mln_gpu_device_capabilities.depth_stencil_formats.
 */
typedef struct mln_open_gl_borrowed_textures_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_open_gl_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  /** The textures' target. It must be `GL_TEXTURE_2D`. */
  uint32_t target MLN_BINDING("required=true");
  const mln_open_gl_borrowed_texture* depth_textures
    MLN_BINDING("length=depth_texture_count");
  size_t depth_texture_count;
} mln_open_gl_borrowed_textures_target;

/** A WebGPU texture of a borrowed ring. */
typedef struct mln_web_gpu_borrowed_texture {
  /** Your `GPUTexture`. */
  mln_web_object texture MLN_BINDING("required=true");
} mln_web_gpu_borrowed_texture;

/**
 * A ring of your WebGPU textures to render into, on a WebGPU device that wraps
 * yours.
 *
 * `physical_width` and `physical_height` give the textures' size in pixels,
 * which is usually `logical_size` times the map's scale factor. Each texture
 * must be 2D with that size, `format`, one layer, one mip level, one sample,
 * and `GPUTextureUsage.RENDER_ATTACHMENT`. The ring holds up to
 * MLN_TEXTURE_RING_MAX_DEPTH distinct textures.
 *
 * Give the ring `depth_textures` to draw into each frame depth-tested against
 * the map, as mln_frame_projection says: one for each of `textures`, at the
 * same index, or none. Each frame clears the one at its slot, renders the
 * map's depth and stencil into it, and stores them. They must be 2D with the
 * size of `textures`, `depth_format`, one layer, one mip level, one sample, and
 * `GPUTextureUsage.RENDER_ATTACHMENT`.
 *
 * The ring renders on a device from mln_gpu_device_wrap_web_gpu(), which no
 * build has yet: an attach with it fails with MLN_STATUS_UNSUPPORTED.
 */
typedef struct mln_web_gpu_borrowed_textures_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_web_gpu_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  /** The textures' `GPUTextureFormat`, such as `rgba8unorm`. */
  mln_buffer_view format MLN_BINDING("required=true");
  const mln_web_gpu_borrowed_texture* depth_textures
    MLN_BINDING("length=depth_texture_count");
  size_t depth_texture_count;
  /**
   * The depth textures' `GPUTextureFormat`, of
   * mln_gpu_device_capabilities.depth_stencil_formats.
   */
  mln_buffer_view depth_format;
} mln_web_gpu_borrowed_textures_target;

/** A DRM format and modifier that you can import. */
typedef struct mln_dma_buf_format {
  /** A DRM fourcc code, such as DRM_FORMAT_ABGR8888. */
  uint32_t fourcc;
  /** A DRM format modifier, such as DRM_FORMAT_MOD_LINEAR. */
  uint64_t modifier;
} mln_dma_buf_format;

/**
 * A ring of textures that the session allocates on its device and shares with
 * you, in one of the ways that you can import.
 *
 * Each texture has the size in pixels of `logical_size` at the map's scale
 * factor. mln_render_session_resize() gives the ring textures of the new size,
 * and a frame that you hold keeps its texture of the old size until you
 * release or discard it.
 *
 * The ring shares its textures in the first kind of `export_kinds` that the
 * device exports, in this order, and
 * mln_render_session_capabilities.export_kind reports it:
 *
 * - MLN_GPU_EXPORT_KIND_IO_SURFACE: IOSurfaces of the pixel format
 *   kCVPixelFormatType_32BGRA, which Metal names MTLPixelFormatBGRA8Unorm.
 *   Get each frame's surface with mln_acquired_frame_get_io_surface().
 * - MLN_GPU_EXPORT_KIND_ANDROID_HARDWARE_BUFFER: hardware buffers of the
 *   format AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, with the usage
 *   AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT,
 *   AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, and the bits of
 *   `android_hardware_buffer_usage`. Get each frame's buffer with
 *   mln_acquired_frame_get_android_hardware_buffer().
 * - MLN_GPU_EXPORT_KIND_DXGI_SHARED_HANDLE: D3D11 textures of the format
 *   DXGI_FORMAT_B8G8R8A8_UNORM on the device's GPU, which bind as shader
 *   resources and render targets. Open them on a D3D11 or D3D12 device of the
 *   same adapter, which you pick with mln_gpu_device_options.adapter_luid.
 *   Get each frame's texture with mln_acquired_frame_get_dxgi_shared_texture().
 * - MLN_GPU_EXPORT_KIND_DMA_BUF: dmabufs of a format and modifier from
 *   `dma_buf_formats`. Get each frame's buffer with
 *   mln_acquired_frame_get_dma_buf().
 * - MLN_GPU_EXPORT_KIND_OPAQUE_FD: the memory of Vulkan images, which only a
 *   Vulkan device or an OpenGL context with GL_EXT_memory_object_fd on the
 *   same GPU and driver imports. To pick the GPU, create the device with
 *   mln_gpu_device_options.adapter_uuid set to your context's
 *   GL_DEVICE_UUID_EXT. Get each frame's memory with
 *   mln_acquired_frame_get_opaque_fd().
 *
 * mln_gpu_device_capabilities.export_kinds reports the kinds that a device
 * exports. Attach with MLN_RENDER_DRIVER_CORE_WORKER. Without a device, the
 * session renders on the first of the platform's backends whose device exports
 * one of `export_kinds`.
 *
 * Each frame hands over with MLN_GPU_SYNC_CPU_COMPLETE or with the sync kind
 * that crosses APIs on its platform: MLN_GPU_SYNC_METAL_SHARED_EVENT for an
 * IOSurface, and MLN_GPU_SYNC_SYNC_FD for the others, where the device has
 * them. For a reader that waits on neither, such as a Wayland compositor
 * without explicit synchronization or Flutter's `copyPixelBuffer`, leave them
 * out of mln_render_session_attach_options.producer_sync_kinds, so that each
 * frame finishes before you acquire it, and release each frame only once the
 * reader is done with its texture. Give the ring 3 textures, so the session
 * can render while you hold two frames.
 */
typedef struct mln_exported_ring_target {
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** How many textures the ring has, from 1 to MLN_TEXTURE_RING_MAX_DEPTH. */
  uint32_t texture_count MLN_BINDING("required=true");
  /** The ways of sharing a texture that you can import. */
  mln_gpu_export_kind_flags export_kinds MLN_BINDING("required=true");
  /**
   * The dmabuf formats and modifiers that you import, most preferred first,
   * such as those that eglQueryDmaBufModifiersEXT() reports. A ring exports
   * dmabufs only of one of them. The library renders only DRM_FORMAT_ABGR8888
   * and DRM_FORMAT_ARGB8888. A modifier can give a buffer more planes than its
   * format has, such as a plane of compression metadata, and the library takes
   * any such modifier that its GPU can render into and share. The library
   * takes no implicit modifier: it skips DRM_FORMAT_MOD_INVALID, so a list of
   * only that one gets no dmabuf.
   */
  const mln_dma_buf_format* dma_buf_formats
    MLN_BINDING("length=dma_buf_format_count");
  size_t dma_buf_format_count;
  /**
   * More `AHARDWAREBUFFER_USAGE_*` bits that your reads of a hardware buffer
   * need, such as AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, or zero.
   */
  uint64_t android_hardware_buffer_usage;
} mln_exported_ring_target;

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
  mln_vulkan_surface_target vulkan_surface MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_VULKAN_SURFACE;"
    "platforms=android,freebsd,linux,macos,ohos,windows"
  );
  mln_open_gl_surface_target open_gl_surface
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_OPEN_GL_SURFACE");
  mln_metal_borrowed_textures_target metal_borrowed_textures MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_METAL_BORROWED_TEXTURES;"
    "platforms=ios,macos,tvos,visionos"
  );
  mln_vulkan_borrowed_textures_target vulkan_borrowed_textures MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_VULKAN_BORROWED_TEXTURES;"
    "platforms=android,freebsd,linux,macos,ohos,windows"
  );
  mln_open_gl_borrowed_textures_target open_gl_borrowed_textures
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_OPEN_GL_BORROWED_TEXTURES");
  mln_web_gpu_borrowed_textures_target web_gpu_borrowed_textures MLN_BINDING(
    "variant=MLN_RENDER_TARGET_KIND_WEB_GPU_BORROWED_TEXTURES;platforms=web"
  );
  mln_exported_ring_target exported_ring
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_EXPORTED_RING");
  mln_web_canvas_target web_canvas
    MLN_BINDING("variant=MLN_RENDER_TARGET_KIND_WEB_CANVAS;platforms=web");
} mln_render_target_data;

/**
 * What a render session renders into: one kind of target, and its
 * descriptor.
 *
 * Every target has a size in logical pixels, which sets the map's size. The
 * session renders it at the map's scale factor. A borrowed or exported ring
 * hands each frame to you: acquire it with mln_render_session_acquire_frame(),
 * and release it with your GPU's synchronization once you have read it.
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
