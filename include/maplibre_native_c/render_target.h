/**
 * @file maplibre_native_c/render_target.h
 * Shared public C API declarations for render target descriptors.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_TARGET_H
#define MAPLIBRE_NATIVE_C_RENDER_TARGET_H

#include <stdint.h>

#include "base.h"
#include "wake.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bit pattern of a Vulkan non-dispatchable handle.
 *
 * Vulkan defines these handles as pointers on 64-bit targets and as uint64_t
 * values on 32-bit targets. This fixed-width carrier preserves either native
 * representation across the C ABI. Zero represents VK_NULL_HANDLE.
 */
typedef uint64_t mln_vulkan_non_dispatchable_handle;

/** Null Vulkan non-dispatchable handle. */
#define MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL UINT64_C(0)

/** Logical render target extent in UI pixels. */
typedef struct mln_render_target_extent {
  uint32_t size;
  /** Logical map width in UI pixels. */
  uint32_t width;
  /** Logical map height in UI pixels. */
  uint32_t height;
  /** UI-to-device pixel scale. Must be positive and finite. */
  double scale_factor;
} mln_render_target_extent;

/** Execution placement for one render session. */
typedef enum mln_render_driver_kind : uint32_t {
  /**
   * Native code owns a serial worker that initializes, drives, and tears down
   * transferable graphics state.
   */
  MLN_RENDER_DRIVER_CORE_WORKER = 1U,
  /**
   * The host explicitly calls the narrow driver API from the thread or realm
   * where its graphics context is current.
   */
  MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD = 2U,
} mln_render_driver_kind;

/** Optional render-session capabilities. */
typedef enum mln_render_session_capability_flag : uint32_t {
  MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION = 1U << 0U,
  MLN_RENDER_SESSION_CAPABILITY_READBACK = 1U << 1U,
  MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC = 1U << 2U,
  MLN_RENDER_SESSION_CAPABILITY_PRESENTATION = 1U << 3U,
} mln_render_session_capability_flag;

/**
 * Common attachment policy copied before an attach call returns.
 *
 * A successful backend attach-start call publishes a session in ATTACHING
 * state and accepts its completion together. Hosts may service caller-driver
 * work through that session before attachment completes. A failed completion
 * still requires normal detach or abandonment before session destruction.
 */
typedef struct mln_render_session_attach_options {
  uint32_t size;
  /** One mln_render_driver_kind value. */
  uint32_t driver;
  /**
   * Requested host-acquirable owned-texture slot count. Private targets grant
   * one slot regardless of this value. Ignored by other targets.
   */
  uint32_t requested_texture_ring_depth;
  uint32_t reserved;
  /** Wakes the receiver when the frame-result queue becomes nonempty. */
  mln_wake frame_wake;
  /** Wakes the graphics receiver when caller-driver work is available. */
  mln_wake driver_work_wake;
} mln_render_session_attach_options;

/** Driver and target capabilities fixed for one attached render session. */
typedef struct mln_render_session_capabilities {
  uint32_t size;
  /** One mln_render_driver_kind value. */
  uint32_t driver;
  /** Granted owned-texture slot count, or zero for a target without a ring. */
  uint32_t texture_ring_depth;
  /** A bitwise OR of mln_render_session_capability_flag values. */
  uint32_t flags;
} mln_render_session_capabilities;

/** Synchronization payload kind for acquired texture frames. */
typedef enum mln_gpu_sync_kind : uint32_t {
  /** The producer or consumer has completed before the API call returns. */
  MLN_GPU_SYNC_CPU_COMPLETE = 0U,
  /** id<MTLSharedEvent> plus a monotonically increasing signal value. */
  MLN_GPU_SYNC_METAL_SHARED_EVENT = 1U,
  /** VkSemaphore plus a timeline value. */
  MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE = 2U,
  /** GLsync, used only by a caller-graphics-thread driver. */
  MLN_GPU_SYNC_OPENGL_FENCE = 3U,
  /** A backend-defined WebGPU completion token. */
  MLN_GPU_SYNC_WEBGPU_TOKEN = 4U,
} mln_gpu_sync_kind;

/**
 * Backend synchronization copied by frame access and release calls.
 *
 * object is retained only for Metal. Vulkan, OpenGL, and WebGPU objects remain
 * borrowed until a later session barrier or detach completes.
 */
typedef struct mln_gpu_sync {
  uint32_t size;
  /** One mln_gpu_sync_kind value. */
  uint32_t kind;
  /**
   * Bit pattern of the backend object that kind names: the id<MTLSharedEvent>
   * pointer, the VkSemaphore handle, the GLsync pointer, or the WebGPU token.
   *
   * A fixed-width carrier keeps a Vulkan non-dispatchable handle intact on
   * 32-bit targets, where it is wider than a pointer. Zero when kind is
   * MLN_GPU_SYNC_CPU_COMPLETE.
   */
  uint64_t object;
  uint64_t value;
} mln_gpu_sync;

/** Returns CPU-complete synchronization for this C API version. */
MLN_API mln_gpu_sync mln_gpu_sync_default(void) MLN_NOEXCEPT;

/** Metal backend context fields shared by Metal render targets. */
typedef struct mln_metal_context_descriptor {
  uint32_t size;
  /** id<MTLDevice> / MTL::Device*. Retained when the target requires it. */
  void* device;
} mln_metal_context_descriptor;

/** Vulkan backend context fields shared by Vulkan render targets. */
typedef struct mln_vulkan_context_descriptor {
  uint32_t size;
  /** Borrowed VkInstance. Required. */
  void* instance;
  /** Borrowed VkPhysicalDevice. Required. */
  void* physical_device;
  /** Borrowed VkDevice. Required. */
  void* device;
  /** Borrowed graphics VkQueue. Required. */
  void* graphics_queue;
  /** Queue family index for graphics_queue. Must support graphics commands. */
  uint32_t graphics_queue_family_index;
  /** PFN_vkGetInstanceProcAddr for the loader that created the Vulkan handles.
   */
  void* get_instance_proc_addr;
  /** PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device. */
  void* get_device_proc_addr;
} mln_vulkan_context_descriptor;

/** WebGPU backend context fields shared by WebGPU render targets. */
typedef struct mln_webgpu_context_descriptor {
  uint32_t size;
  /** Borrowed WGPUInstance. Optional for texture sessions. */
  void* instance;
  /** Borrowed WGPUDevice. Required. */
  void* device;
  /**
   * Borrowed WGPUQueue. Optional; null uses the device default queue. A
   * non-null queue must belong to device.
   */
  void* queue;
} mln_webgpu_context_descriptor;

/** OpenGL context providers supported by this build. */
typedef enum mln_opengl_context_provider_flag : uint32_t {
  MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL = 1u << 0u,
  MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL = 1u << 1u,
  /** Browser WebGL context imported into an Emscripten module. */
  MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL = 1u << 2u,
} mln_opengl_context_provider_flag;

/** OpenGL platform context provider used by a context descriptor. */
typedef enum mln_opengl_context_platform : uint32_t {
  /** No OpenGL context provider is selected. */
  MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED = 0u,
  MLN_OPENGL_CONTEXT_PLATFORM_WGL = 1u,
  MLN_OPENGL_CONTEXT_PLATFORM_EGL = 2u,
  /** Emscripten WebGL context handle. */
  MLN_OPENGL_CONTEXT_PLATFORM_WEBGL = 3u,
} mln_opengl_context_platform;

/**
 * How a session's OpenGL context relates to its driver thread and host graphics
 * state.
 *
 * A shared session leaves the thread as it found it: every render makes the
 * session context current and restores whatever was current before. The session
 * context joins the host share group named by the descriptor, so a host may
 * hand the session a texture and sample it from its own context.
 *
 * A dedicated session owns its driver thread's context. It makes its context
 * current once, keeps it current between renders, and joins no share group. A
 * core worker can own transferable private graphics state. A host thread can
 * own a surface context, such as an Android SurfaceView context.
 */
typedef enum mln_opengl_context_ownership : uint32_t {
  /** The session shares its thread with host graphics work. */
  MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED = 0u,
  /** The session owns its thread's OpenGL context. */
  MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED = 1u,
} mln_opengl_context_ownership;

/** OpenGL client API a dedicated EGL session creates its context for. */
typedef enum mln_opengl_client_api : uint32_t {
  /** No client API is named. */
  MLN_OPENGL_CLIENT_API_UNSPECIFIED = 0u,
  /** Desktop OpenGL, as EGL_OPENGL_API names it. */
  MLN_OPENGL_CLIENT_API_GL = 1u,
  /** OpenGL ES, as EGL_OPENGL_ES_API names it. */
  MLN_OPENGL_CLIENT_API_GLES = 2u,
} mln_opengl_client_api;

/** WGL context fields shared by OpenGL render targets on Windows. */
typedef struct mln_wgl_context_descriptor {
  uint32_t size;
  /** Borrowed HDC used to create the session context. Required. */
  void* device_context;
  /**
   * Borrowed HGLRC whose share group the session context joins. Required under
   * shared ownership. A dedicated session joins no share group, so it must be
   * null there.
   */
  void* share_context;
  /** Optional wglGetProcAddress-compatible function for the host loader. */
  void* get_proc_address;
} mln_wgl_context_descriptor;

/** EGL context fields shared by OpenGL render targets. */
typedef struct mln_egl_context_descriptor {
  uint32_t size;
  /** Borrowed EGLDisplay. Required and kept initialized through teardown. */
  void* display;
  /**
   * Borrowed EGLConfig used to create the session context. Required.
   * OpenGL texture sessions require EGL_SURFACE_TYPE to include
   * EGL_PBUFFER_BIT.
   */
  void* config;
  /**
   * Borrowed EGLContext whose share group the session context joins. Required
   * under shared ownership, where the session also takes its client API from
   * this context. A dedicated session joins no share group, so it must be null
   * there and names client_api instead.
   */
  void* share_context;
  /**
   * Client API the session creates its context for. Required under dedicated
   * ownership. A shared session queries share_context for it, so this is
   * ignored there.
   */
  mln_opengl_client_api client_api;
  /** Optional eglGetProcAddress-compatible function for the host loader. */
  void* get_proc_address;
} mln_egl_context_descriptor;

/** WebGL context placement. */
typedef enum mln_webgl_context_kind : uint32_t {
  /** Use a host-created context on its current browser agent. */
  MLN_WEBGL_CONTEXT_EXISTING = 0U,
  /**
   * Create a WebGL 2 context on a native worker whose pthread creation claims
   * canvas_selector through Emscripten's transferred-canvases attribute.
   */
  MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS = 1U,
} mln_webgl_context_kind;

/** WebGL context fields shared by OpenGL render targets in the browser. */
typedef struct mln_webgl_context_descriptor {
  uint32_t size;
  /** One mln_webgl_context_kind value. */
  uint32_t kind;
  /** Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
   */
  int32_t context;
  /**
   * Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
   * canvas must still be transferable when attachment starts.
   */
  mln_buffer_view canvas_selector;
} mln_webgl_context_descriptor;

/** OpenGL backend context fields shared by OpenGL render targets. */
typedef struct mln_opengl_context_descriptor {
  uint32_t size;
  /** WGL, EGL, or WebGL context provider. */
  mln_opengl_context_platform platform;
  /**
   * Whether the session shares its driver thread and graphics objects with the
   * host. A private EGL owned texture and a transferred WebGL canvas are
   * dedicated to their core worker.
   */
  mln_opengl_context_ownership ownership;
  union {
    mln_wgl_context_descriptor wgl;
    mln_egl_context_descriptor egl;
    mln_webgl_context_descriptor webgl;
  } data;
} mln_opengl_context_descriptor;

/** Returns default core-worker attachment policy with inherited completions. */
MLN_API mln_render_session_attach_options
mln_render_session_attach_options_default(void) MLN_NOEXCEPT;

/**
 * Computes the physical device-pixel size of a logical render target extent.
 *
 * Each dimension is ceil(logical * scale_factor). Session-owned texture targets
 * and surface targets are sized this way. Caller-owned borrowed texture targets
 * state their physical size directly; see texture.h.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when extent is null or invalid, out_width or
 *   out_height is null, or the scaled dimensions are too large.
 */
MLN_API mln_status mln_render_target_extent_physical_size(
  const mln_render_target_extent* extent, uint32_t* out_width,
  uint32_t* out_height
) MLN_NOEXCEPT;

/**
 * Returns OpenGL context providers supported by this build.
 */
MLN_API uint32_t mln_opengl_supported_context_provider_mask(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_TARGET_H
