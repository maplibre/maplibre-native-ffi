/**
 * @file maplibre_native_c/render_target.h
 * Shared public C API declarations for render target descriptors.
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

/**
 * Bit pattern of a Vulkan non-dispatchable handle.
 *
 * It carries either native representation of the handle, a pointer or a
 * uint64_t. Zero represents VK_NULL_HANDLE.
 */
typedef uint64_t mln_vulkan_non_dispatchable_handle;

/** Null Vulkan non-dispatchable handle. */
#define MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL UINT64_C(0)

/** The most slots a texture ring holds, owned or borrowed. */
#define MLN_TEXTURE_RING_MAX_DEPTH 8

/** Execution placement for one render session. */
typedef enum mln_render_driver_kind : uint32_t {
  /**
   * Native code initializes, services, and tears down transferable graphics
   * state on a library-owned thread.
   */
  MLN_RENDER_DRIVER_CORE_WORKER = 1U,
  /**
   * The host services driver work on the thread or realm where its graphics
   * context is current.
   */
  MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD = 2U,
} mln_render_driver_kind;

/** Optional render-session capabilities. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_render_session_capability_flag : uint32_t {
  MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION = 1U << 0U,
  MLN_RENDER_SESSION_CAPABILITY_READBACK = 1U << 1U,
  MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC = 1U << 2U,
  MLN_RENDER_SESSION_CAPABILITY_PRESENTATION = 1U << 3U,
} mln_render_session_capability_flag;

/** A set of mln_render_session_capability_flag bits. */
typedef uint32_t mln_render_session_capability_flags;

/**
 * Takes or returns the host's lock on the queue that a session shares with it.
 * Native code relies on the lock being held or released when the call returns.
 */
MLN_BINDING("synchronous=true")
typedef void (*mln_queue_lock_callback)(void* user_data);

/**
 * Host lock on the graphics queue that a session shares with its host, copied
 * by a successful attach.
 *
 * Vulkan requires the host to synchronize every call that uses a queue. A
 * Vulkan session submits to the context's graphics_queue. A host that also
 * uses that queue must pass a lock here and take the same lock around its own
 * queue calls. The callbacks may run on the graphics thread or the thread that
 * abandons the session. Each queue submission or presentation holds the lock
 * only for that call, never across a wait.
 *
 * Both callbacks are null to disable the lock, or both are set. A disabled
 * lock must not carry release_user_data. A session without a lock assumes that
 * the host leaves the queue alone while the session is attached.
 *
 * The callbacks must not call the C API; each status-returning function
 * returns MLN_STATUS_INVALID_STATE when a callback calls it. The host
 * must not hold the lock while calling the C API or waiting on a thread that
 * does. Sessions sharing a queue must use the same host mutex.
 *
 * Native code calls release_user_data once, after the session can no longer
 * invoke the callbacks.
 */
typedef struct mln_queue_lock {
  mln_queue_lock_callback lock MLN_BINDING("nullable=true");
  mln_queue_lock_callback unlock MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data once, after all callback invocations have
   * returned and the session can no longer invoke the callbacks. A disabled
   * lock leaves it null.
   */
  mln_user_data_release release_user_data;
} mln_queue_lock MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** Synchronization payload kind for acquired texture frames. */
typedef enum mln_gpu_sync_kind : uint32_t {
  /**
   * The host needs no synchronization object. The work completed, or on WebGPU
   * was submitted to the device's queue, before the frame became acquirable or
   * before the release call.
   *
   * As producer synchronization of an acquired frame: on Metal, Vulkan, and
   * OpenGL, the producer's GPU writes to the frame's texture completed before
   * the frame became acquirable. On OpenGL, a host that samples the texture
   * from another context of the share group binds it again after acquisition.
   * On WebGPU, the writes were submitted to the device's queue, which orders
   * them before any work that the host submits afterwards.
   *
   * As consumer synchronization: the host's GPU reads completed before the
   * release call, or on WebGPU were submitted to the device's queue before it.
   */
  MLN_GPU_SYNC_CPU_COMPLETE = 0U,
  /**
   * GLsync in the shared EGL or WGL context's share group, used only by a
   * caller-graphics-thread driver. handle is nonzero.
   *
   * A producer fence is a flushed GL_SYNC_GPU_COMMANDS_COMPLETE fence. Before
   * reading the texture,
   * the host queues glWaitSync(handle, 0, GL_TIMEOUT_IGNORED) in a context in
   * that share group and binds the texture again. A CPU read instead waits
   * with glClientWaitSync. EGL targets use GLsync, not EGLSync.
   *
   * The session owns producer fences. The host borrows one until it releases
   * or discards the acquired frame and must never delete it. A view scope
   * extends that borrowing.
   *
   * The host inserts a consumer fence after its reads and calls glFlush in
   * that context before releasing the frame. A successful release transfers
   * the fence to the session, which waits on it before it renders into the
   * slot again.
   *
   * Target loss invalidates frame access and may retain consumer fences until
   * process exit. Abandon performs no graphics cleanup and may retain producer
   * and consumer fences until process exit.
   * The host finishes its GPU reads before destroying borrowed textures.
   */
  MLN_GPU_SYNC_OPEN_GL_FENCE = 1U,
} mln_gpu_sync_kind;

/**
 * Common attachment policy copied before an attach call returns.
 *
 * Every backend attachment call requires *out_session to be MLN_HANDLE_NULL
 * on entry. MLN_STATUS_OK transfers an ATTACHING session to the caller and
 * accepts its completion. A non-OK return leaves *out_session unchanged and
 * invokes no completion. Hosts may service caller-driver work before
 * attachment completes. After a failed completion, the host still disposes
 * the session with mln_render_session_dispose().
 */
typedef struct mln_render_session_attach_options {
  uint32_t size;
  /** Where the session's driver runs. Required. */
  mln_render_driver_kind driver MLN_BINDING("required=true");
  /**
   * Requested slot count of a session-owned texture ring, up to
   * MLN_TEXTURE_RING_MAX_DEPTH. Zero selects 2, so the host can retain one
   * frame while the session renders its replacement. Private targets grant
   * one slot regardless of this value. A borrowed texture ring's depth is its
   * texture count, so borrowed and other targets ignore this value.
   */
  uint32_t requested_texture_ring_depth;
  /**
   * Producer synchronization for acquired texture frames.
   * MLN_GPU_SYNC_OPEN_GL_FENCE requires a shared EGL or WGL texture target and
   * a caller-graphics-thread driver. Other targets support only CPU completion
   * and fail an unsupported request.
   */
  mln_gpu_sync_kind producer_sync_kind;
  /** Wakes the receiver when the frame-result queue becomes nonempty. */
  mln_wake frame_wake;
  /**
   * Wakes the graphics receiver when caller-driver work is available,
   * including work that a finished driver result releases.
   *
   * The host services that work whenever the wake fires, even while it renders
   * no frames, because attach, detach, and queries complete only through
   * service.
   */
  mln_wake driver_work_wake;
  /**
   * Host lock on the graphics queue, disabled by default. Only Vulkan targets
   * accept an enabled lock; other backends fail the attach with
   * MLN_STATUS_UNSUPPORTED.
   */
  mln_queue_lock queue_lock;
} mln_render_session_attach_options;

/** Driver and target capabilities fixed for one attached render session. */
typedef struct mln_render_session_capabilities {
  uint32_t size;
  mln_render_driver_kind driver;
  /**
   * Granted texture ring depth: the slot count of a session-owned ring, or the
   * texture count of a borrowed one. Zero for a surface.
   */
  uint32_t texture_ring_depth;
  mln_render_session_capability_flags flags;
} mln_render_session_capabilities;

/**
 * Backend synchronization copied by frame access and release calls.
 *
 * OpenGL ownership and lifetimes follow MLN_GPU_SYNC_OPEN_GL_FENCE.
 */
typedef struct mln_gpu_sync {
  uint32_t size;
  mln_gpu_sync_kind kind;
  /**
   * Bit pattern of the GLsync pointer.
   *
   * Zero when kind is MLN_GPU_SYNC_CPU_COMPLETE. A producer fence that frame
   * access copies is valid only until the frame's release or discard.
   */
  uint64_t handle MLN_BINDING("lifetime=owner");
} mln_gpu_sync;

/** Metal backend context fields shared by Metal render targets. */
typedef struct mln_metal_context_descriptor {
  /** `id<MTLDevice>` / `MTL::Device*`. Retained when the target requires it. */
  void* device;
} mln_metal_context_descriptor;

/** Vulkan backend context fields shared by Vulkan render targets. */
typedef struct mln_vulkan_context_descriptor {
  /** Borrowed VkInstance. Required. */
  void* instance MLN_BINDING("required=true");
  /** Borrowed VkPhysicalDevice. Required. */
  void* physical_device MLN_BINDING("required=true");
  /** Borrowed VkDevice. Required. */
  void* device MLN_BINDING("required=true");
  /**
   * Borrowed graphics VkQueue. Required. The session's driver submits to it
   * from its own thread, so a host that uses the same queue passes
   * mln_render_session_attach_options.queue_lock at attach.
   */
  void* graphics_queue MLN_BINDING("required=true");
  /** Queue family index for graphics_queue. Must support graphics commands. */
  uint32_t graphics_queue_family_index;
  /** PFN_vkGetInstanceProcAddr for the loader that created the instance. */
  void* get_instance_proc_addr;
  /** PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device. */
  void* get_device_proc_addr;
} mln_vulkan_context_descriptor;

/** WebGPU backend context fields shared by WebGPU render targets. */
typedef struct mln_web_gpu_context_descriptor {
  /** Borrowed WGPUInstance. Optional for texture targets. */
  void* instance;
  /** Borrowed WGPUDevice. Required. */
  void* device MLN_BINDING("required=true");
  /**
   * Borrowed WGPUQueue. Optional; null uses the device default queue. A
   * non-null queue must belong to device.
   */
  void* queue;
} mln_web_gpu_context_descriptor;

/** OpenGL context providers supported by this build. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_open_gl_context_provider_flag : uint32_t {
  MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL = 1u << 0u,
  MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL = 1u << 1u,
  /** Browser WebGL context imported into an Emscripten module. */
  MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL = 1u << 2u,
} mln_open_gl_context_provider_flag;

/** A set of mln_open_gl_context_provider_flag bits. */
typedef uint32_t mln_open_gl_context_provider_flags;

/** OpenGL platform context provider used by a context descriptor. */
typedef enum mln_open_gl_context_platform : uint32_t {
  /** No OpenGL context provider is selected. */
  MLN_OPEN_GL_CONTEXT_PLATFORM_UNSPECIFIED = 0u,
  MLN_OPEN_GL_CONTEXT_PLATFORM_WGL = 1u,
  MLN_OPEN_GL_CONTEXT_PLATFORM_EGL = 2u,
  /** Emscripten WebGL context handle. */
  MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL = 3u,
} mln_open_gl_context_platform;

/**
 * How a session's OpenGL context relates to its driver thread and host graphics
 * state.
 *
 * A shared session leaves the thread as it found it: every render makes the
 * session context current and restores whatever was current before. The session
 * context joins the host share group named by the descriptor, so a host may
 * hand the session a texture and sample it from its own context.
 *
 * A dedicated session joins no share group and leaves its context current
 * after a render instead of restoring what was current before. A render that
 * finds another context current makes its own current again, so several
 * dedicated sessions can share one thread, and the host can use its own
 * context between renders. Teardown releases the session context from the
 * thread and restores nothing.
 */
typedef enum mln_open_gl_context_ownership : uint32_t {
  /** The session shares its thread with host graphics work. */
  MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED = 0u,
  /** The session keeps its own OpenGL context current on its thread. */
  MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED = 1u,
} mln_open_gl_context_ownership;

/** OpenGL client API a dedicated EGL session creates its context for. */
typedef enum mln_open_gl_client_api : uint32_t {
  /** No client API is named. */
  MLN_OPEN_GL_CLIENT_API_UNSPECIFIED = 0u,
  /** Desktop OpenGL, as EGL_OPENGL_API names it. */
  MLN_OPEN_GL_CLIENT_API_GL = 1u,
  /** OpenGL ES, as EGL_OPENGL_ES_API names it. */
  MLN_OPEN_GL_CLIENT_API_GLES = 2u,
} mln_open_gl_client_api;

/** WGL context fields shared by OpenGL render targets on Windows. */
typedef struct mln_wgl_context_descriptor {
  /**
   * Borrowed HDC used to create the session context. Required. A dedicated
   * owned texture makes its context current on this HDC from its core worker.
   */
  void* device_context MLN_BINDING("required=true");
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
  /**
   * Borrowed EGLDisplay. Required and kept initialized through teardown.
   * eglGetDisplay(EGL_DEFAULT_DISPLAY) returns the same display for the whole
   * process, so a shared display must stay initialized until every session
   * using it has detached or been destroyed. The host must not call
   * eglTerminate per session.
   */
  void* display MLN_BINDING("required=true");
  /**
   * Borrowed EGLConfig used to create the session context. Required.
   * OpenGL texture targets require EGL_SURFACE_TYPE to include
   * EGL_PBUFFER_BIT.
   */
  void* config MLN_BINDING("required=true");
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
  mln_open_gl_client_api client_api;
  /** Optional eglGetProcAddress-compatible function for the host loader. */
  void* get_proc_address;
} mln_egl_context_descriptor;

/** WebGL context placement. */
typedef enum mln_web_gl_context_kind : uint32_t {
  /** Use a host-created context on its current browser agent. */
  MLN_WEB_GL_CONTEXT_EXISTING = 0U,
  /**
   * Transfer the canvas named by canvas_selector to a native worker and create
   * a WebGL 2 context there.
   */
  MLN_WEB_GL_CONTEXT_TRANSFERRED_CANVAS = 1U,
} mln_web_gl_context_kind;

/** WebGL context fields shared by OpenGL render targets in the browser. */
typedef struct mln_web_gl_context_descriptor {
  mln_web_gl_context_kind kind;
  /**
   * Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE for EXISTING. Must be positive.
   */
  int32_t context;
  /**
   * Copied UTF-8 Emscripten target selector for TRANSFERRED_CANVAS. The HTML
   * canvas must still be transferable when attachment starts.
   */
  mln_buffer_view canvas_selector;
} mln_web_gl_context_descriptor;

/** Backend-specific OpenGL context data. */
typedef union mln_open_gl_context_descriptor_data {
  mln_wgl_context_descriptor wgl
    MLN_BINDING("variant=MLN_OPEN_GL_CONTEXT_PLATFORM_WGL");
  mln_egl_context_descriptor egl
    MLN_BINDING("variant=MLN_OPEN_GL_CONTEXT_PLATFORM_EGL");
  mln_web_gl_context_descriptor web_gl
    MLN_BINDING("variant=MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL");
} mln_open_gl_context_descriptor_data;

/** OpenGL backend context fields shared by OpenGL render targets. */
typedef struct mln_open_gl_context_descriptor {
  /** WGL, EGL, or WebGL context provider. Required. */
  mln_open_gl_context_platform platform;
  /**
   * Whether the session shares its driver thread and graphics objects with the
   * host. A private EGL or WGL owned texture and a transferred WebGL canvas are
   * dedicated to their core worker.
   */
  mln_open_gl_context_ownership ownership;
  mln_open_gl_context_descriptor_data data MLN_BINDING("tag=platform");
} mln_open_gl_context_descriptor;

/**
 * Computes the physical device-pixel size of a logical extent.
 *
 * Each dimension is ceil(logical * scale_factor). Session-owned texture targets
 * and surface targets are sized this way. Caller-owned borrowed texture targets
 * state their physical size directly; see texture.h.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when extent has a zero width or height, or a
 *   scale_factor that is not finite and positive; out_width or out_height is
 *   null; or a scaled dimension exceeds UINT32_MAX.
 */
MLN_API mln_status mln_logical_extent_physical_size(
  mln_logical_extent extent, uint32_t* out_width MLN_BINDING("direction=out"),
  uint32_t* out_height MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Returns OpenGL context providers supported by this build.
 */
MLN_API mln_open_gl_context_provider_flags
mln_open_gl_supported_context_provider_mask(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_TARGET_H
