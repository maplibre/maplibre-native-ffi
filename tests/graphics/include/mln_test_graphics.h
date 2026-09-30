#ifndef MLN_TEST_GRAPHICS_H
#define MLN_TEST_GRAPHICS_H

// GPU contexts, textures, and surfaces for tests, in the shapes that the render
// target descriptors in maplibre_native_c/ take. The C suite and every binding
// suite create their graphics objects here, so that one implementation stands
// in for the host that owns them.
//
// The library loads each graphics API at run time and links none, so a caller
// can load it from any language over its C ABI. MLN_FFI_VULKAN_LOADER_DIR names
// the directory of the Vulkan loader to use ahead of the default search.
//
// A graphics object and everything created from it belong to one thread at a
// time. Destroy textures and surfaces before the graphics object that created
// them. A function that fails records why for the calling thread, and
// mln_test_graphics_last_error() returns that text.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(MLN_TEST_GRAPHICS_EXPORTS)
#define MLN_TEST_GRAPHICS_API __declspec(dllexport)
#elif defined(__GNUC__)
#define MLN_TEST_GRAPHICS_API __attribute__((visibility("default")))
#else
#define MLN_TEST_GRAPHICS_API
#endif

#if defined(__clang__)
#define MLN_TEST_GRAPHICS_NULLABLE _Nullable
#define MLN_TEST_GRAPHICS_NONNULL _Nonnull
#else
#define MLN_TEST_GRAPHICS_NULLABLE
#define MLN_TEST_GRAPHICS_NONNULL
#endif

#ifdef __cplusplus
extern "C" {
#endif

// The graphics API that a graphics object drives.
enum {
  MLN_TEST_GRAPHICS_BACKEND_METAL = 1,
  MLN_TEST_GRAPHICS_BACKEND_VULKAN = 2,
  MLN_TEST_GRAPHICS_BACKEND_EGL = 3,
  MLN_TEST_GRAPHICS_BACKEND_WGL = 4,
};

typedef struct mln_test_graphics mln_test_graphics;
typedef struct mln_test_graphics_texture mln_test_graphics_texture;
typedef struct mln_test_graphics_surface mln_test_graphics_surface;

// The handles that a backend's context descriptor takes. Only the fields of
// the graphics object's backend are set; the rest are zero.
typedef struct mln_test_graphics_context {
  uint32_t backend;
  // Vulkan: the family of vulkan_queue, which supports graphics commands.
  uint32_t vulkan_queue_family_index;
  // Metal: id<MTLDevice>.
  void* MLN_TEST_GRAPHICS_NULLABLE metal_device;
  // Vulkan: VkInstance, VkPhysicalDevice, VkDevice, and a graphics VkQueue.
  void* MLN_TEST_GRAPHICS_NULLABLE vulkan_instance;
  void* MLN_TEST_GRAPHICS_NULLABLE vulkan_physical_device;
  void* MLN_TEST_GRAPHICS_NULLABLE vulkan_device;
  void* MLN_TEST_GRAPHICS_NULLABLE vulkan_queue;
  // Vulkan: PFN_vkGetInstanceProcAddr and PFN_vkGetDeviceProcAddr of the
  // loader that created the handles above.
  void* MLN_TEST_GRAPHICS_NULLABLE vulkan_get_instance_proc_addr;
  void* MLN_TEST_GRAPHICS_NULLABLE vulkan_get_device_proc_addr;
  // EGL: EGLDisplay, and an EGLConfig with EGL_PBUFFER_BIT and OpenGL ES 3.
  void* MLN_TEST_GRAPHICS_NULLABLE egl_display;
  void* MLN_TEST_GRAPHICS_NULLABLE egl_config;
  // EGL: an OpenGL ES 3 EGLContext on egl_config, for a shared session to
  // join. A dedicated session takes the display and config alone.
  void* MLN_TEST_GRAPHICS_NULLABLE egl_context;
  // WGL: the HDC of a hidden window, and an HGLRC created on it.
  void* MLN_TEST_GRAPHICS_NULLABLE wgl_device_context;
  void* MLN_TEST_GRAPHICS_NULLABLE wgl_context;
  // EGL and WGL: eglGetProcAddress or wglGetProcAddress.
  void* MLN_TEST_GRAPHICS_NULLABLE get_proc_address;
} mln_test_graphics_context;

// A texture that a borrowed texture descriptor can name. Only the fields of
// the texture's backend are set; the rest are zero.
typedef struct mln_test_graphics_texture_info {
  // Vulkan: VkImage and a 2D color VkImageView of it, as bit patterns.
  uint64_t vulkan_image;
  uint64_t vulkan_image_view;
  // Metal: id<MTLTexture>, single-sample, with render-target usage.
  void* MLN_TEST_GRAPHICS_NULLABLE metal_texture;
  // Physical size in pixels.
  uint32_t width;
  uint32_t height;
  // MTLPixelFormat, VkFormat, or the OpenGL internal format.
  uint32_t format;
  // Vulkan: the layouts to name as the descriptor's initial_layout and
  // final_layout. mln_test_graphics_texture_read_rgba8() expects the image in
  // the final layout.
  uint32_t vulkan_initial_layout;
  uint32_t vulkan_final_layout;
  // EGL and WGL: the texture name, and its target, GL_TEXTURE_2D.
  uint32_t opengl_texture;
  uint32_t opengl_target;
} mln_test_graphics_texture_info;

// A presentation surface that a surface descriptor can name. Only the fields
// of the surface's backend are set; the rest are zero.
typedef struct mln_test_graphics_surface_info {
  // Vulkan: VkSurfaceKHR bit pattern, presentable from the context's queue.
  uint64_t vulkan_surface;
  // Metal: CAMetalLayer* on the context's device.
  void* MLN_TEST_GRAPHICS_NULLABLE metal_layer;
  // EGL: a pbuffer EGLSurface on the context's config. WGL: the HDC of a
  // hidden window with the context's pixel format.
  void* MLN_TEST_GRAPHICS_NULLABLE opengl_surface;
  uint32_t width;
  uint32_t height;
} mln_test_graphics_surface_info;

// Returns the reason the calling thread's last failed call failed.
MLN_TEST_GRAPHICS_API const char* MLN_TEST_GRAPHICS_NONNULL
mln_test_graphics_last_error(void);

// Creates a device or context for one MLN_TEST_GRAPHICS_BACKEND_* value, or
// returns null when this platform or host cannot provide one. The created
// context is not current on any thread.
MLN_TEST_GRAPHICS_API mln_test_graphics* MLN_TEST_GRAPHICS_NULLABLE
mln_test_graphics_create(uint32_t backend);
MLN_TEST_GRAPHICS_API void mln_test_graphics_destroy(
  mln_test_graphics* MLN_TEST_GRAPHICS_NULLABLE graphics
);

MLN_TEST_GRAPHICS_API bool mln_test_graphics_get_context(
  const mln_test_graphics* MLN_TEST_GRAPHICS_NONNULL graphics,
  mln_test_graphics_context* MLN_TEST_GRAPHICS_NONNULL out_context
);

// Makes the EGL or WGL context current on the calling thread, as a host that
// drives a session on its own graphics thread would. Fails for Metal and
// Vulkan, which have no current context.
MLN_TEST_GRAPHICS_API bool mln_test_graphics_make_current(
  mln_test_graphics* MLN_TEST_GRAPHICS_NONNULL graphics
);

// Creates a width x height RGBA color texture that the host owns and a
// session can render into.
MLN_TEST_GRAPHICS_API mln_test_graphics_texture* MLN_TEST_GRAPHICS_NULLABLE
mln_test_graphics_texture_create(
  mln_test_graphics* MLN_TEST_GRAPHICS_NONNULL graphics, uint32_t width,
  uint32_t height
);
MLN_TEST_GRAPHICS_API void mln_test_graphics_texture_destroy(
  mln_test_graphics_texture* MLN_TEST_GRAPHICS_NULLABLE texture
);
MLN_TEST_GRAPHICS_API bool mln_test_graphics_texture_get_info(
  const mln_test_graphics_texture* MLN_TEST_GRAPHICS_NONNULL texture,
  mln_test_graphics_texture_info* MLN_TEST_GRAPHICS_NONNULL out_info
);

// Copies the texture's pixels into `pixels` as 8-bit RGBA, row 0 of the
// texture first. Which row of a rendered map lands in row 0 depends on the
// backend. `size` must be at least width * height * 4. Call it only while no
// session is rendering into the texture, because the copy runs on the
// context's own queue or context.
MLN_TEST_GRAPHICS_API bool mln_test_graphics_texture_read_rgba8(
  mln_test_graphics_texture* MLN_TEST_GRAPHICS_NONNULL texture,
  uint8_t* MLN_TEST_GRAPHICS_NONNULL pixels, size_t size
);

// Creates a width x height presentation surface with no window on screen.
MLN_TEST_GRAPHICS_API mln_test_graphics_surface* MLN_TEST_GRAPHICS_NULLABLE
mln_test_graphics_surface_create(
  mln_test_graphics* MLN_TEST_GRAPHICS_NONNULL graphics, uint32_t width,
  uint32_t height
);
MLN_TEST_GRAPHICS_API void mln_test_graphics_surface_destroy(
  mln_test_graphics_surface* MLN_TEST_GRAPHICS_NULLABLE surface
);
MLN_TEST_GRAPHICS_API bool mln_test_graphics_surface_get_info(
  const mln_test_graphics_surface* MLN_TEST_GRAPHICS_NONNULL surface,
  mln_test_graphics_surface_info* MLN_TEST_GRAPHICS_NONNULL out_info
);

#ifdef __cplusplus
}
#endif

#endif
