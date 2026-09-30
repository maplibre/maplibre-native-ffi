// Loads each graphics API at run time, so the library links no graphics
// implementation and a build of it runs wherever the API's loader is present.
// Backends a platform cannot have compile to a failure that names the reason.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mln_test_graphics.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>

#if defined(__APPLE__)
#define MLN_TG_HAS_METAL 1
#endif
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__)
#define MLN_TG_HAS_EGL 1
#endif
#if defined(_WIN32)
#define MLN_TG_HAS_WGL 1
#endif
#if defined(MLN_TG_HAS_EGL) || defined(MLN_TG_HAS_WGL)
#define MLN_TG_HAS_GL 1
#endif

#if defined(_WIN32) && !defined(_WIN64)
#define MLN_TG_GL_API __stdcall
#else
#define MLN_TG_GL_API
#endif

#if defined(_MSC_VER)
#define MLN_TG_THREAD_LOCAL __declspec(thread)
#else
#define MLN_TG_THREAD_LOCAL _Thread_local
#endif

// Errors and libraries

static MLN_TG_THREAD_LOCAL char last_error[512];

static void set_error(const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  vsnprintf(last_error, sizeof(last_error), format, arguments);
  va_end(arguments);
}

const char* mln_test_graphics_last_error(void) { return last_error; }

// Graphics libraries stay loaded for the rest of the process once opened. A
// driver may register process-exit work from inside its image, and unloading
// the image would leave that work pointing at unmapped code.
static void* library_open(const char* path) {
#if defined(_WIN32)
  return (void*)LoadLibraryA(path);
#else
  return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

static void* library_symbol(void* library, const char* name) {
  if (library == NULL) return NULL;
#if defined(_WIN32)
  return (void*)(uintptr_t)GetProcAddress((HMODULE)library, name);
#else
  return dlsym(library, name);
#endif
}

static const char* library_error(void) {
#if defined(_WIN32)
  static MLN_TG_THREAD_LOCAL char message[32];
  snprintf(message, sizeof(message), "error %lu", GetLastError());
  return message;
#else
  const char* message = dlerror();
  return message != NULL ? message : "not found";
#endif
}

// Opens the first of `candidates` that loads, and records every failure.
static void* library_open_first(
  const char* what, const char* const* candidates, size_t count
) {
  char failures[384] = {0};
  size_t used = 0;
  for (size_t index = 0; index < count; index += 1) {
    if (candidates[index] == NULL || candidates[index][0] == '\0') continue;
    void* library = library_open(candidates[index]);
    if (library != NULL) return library;
    const int written = snprintf(
      failures + used, sizeof(failures) - used, "%s%s: %s",
      used == 0 ? "" : "; ", candidates[index], library_error()
    );
    if (written > 0 && used + (size_t)written < sizeof(failures)) {
      used += (size_t)written;
    }
  }
  set_error("could not load %s (%s)", what, failures);
  return NULL;
}

// Objects

typedef struct vulkan_functions {
  PFN_vkGetInstanceProcAddr get_instance_proc_addr;
  PFN_vkGetDeviceProcAddr get_device_proc_addr;
  PFN_vkDestroyInstance destroy_instance;
  PFN_vkGetPhysicalDeviceMemoryProperties get_memory_properties;
  PFN_vkDestroySurfaceKHR destroy_surface;
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR get_surface_support;
  PFN_vkCreateHeadlessSurfaceEXT create_headless_surface;
  PFN_vkVoidFunction create_metal_surface;
  PFN_vkVoidFunction create_win32_surface;
  PFN_vkVoidFunction create_android_surface;
  PFN_vkDestroyDevice destroy_device;
  PFN_vkDeviceWaitIdle device_wait_idle;
  PFN_vkCreateImage create_image;
  PFN_vkDestroyImage destroy_image;
  PFN_vkGetImageMemoryRequirements get_image_memory_requirements;
  PFN_vkBindImageMemory bind_image_memory;
  PFN_vkCreateImageView create_image_view;
  PFN_vkDestroyImageView destroy_image_view;
  PFN_vkAllocateMemory allocate_memory;
  PFN_vkFreeMemory free_memory;
  PFN_vkMapMemory map_memory;
  PFN_vkUnmapMemory unmap_memory;
  PFN_vkCreateBuffer create_buffer;
  PFN_vkDestroyBuffer destroy_buffer;
  PFN_vkGetBufferMemoryRequirements get_buffer_memory_requirements;
  PFN_vkBindBufferMemory bind_buffer_memory;
  PFN_vkCreateCommandPool create_command_pool;
  PFN_vkDestroyCommandPool destroy_command_pool;
  PFN_vkAllocateCommandBuffers allocate_command_buffers;
  PFN_vkFreeCommandBuffers free_command_buffers;
  PFN_vkBeginCommandBuffer begin_command_buffer;
  PFN_vkEndCommandBuffer end_command_buffer;
  PFN_vkCmdPipelineBarrier cmd_pipeline_barrier;
  PFN_vkCmdCopyImageToBuffer cmd_copy_image_to_buffer;
  PFN_vkQueueSubmit queue_submit;
  PFN_vkQueueWaitIdle queue_wait_idle;
} vulkan_functions;

typedef struct gl_functions {
  void(MLN_TG_GL_API* gen_textures)(int32_t, uint32_t*);
  void(MLN_TG_GL_API* delete_textures)(int32_t, const uint32_t*);
  void(MLN_TG_GL_API* bind_texture)(uint32_t, uint32_t);
  void(MLN_TG_GL_API* tex_image_2d)(
    uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, uint32_t,
    const void*
  );
  void(MLN_TG_GL_API* tex_parameteri)(uint32_t, uint32_t, int32_t);
  void(MLN_TG_GL_API* gen_framebuffers)(int32_t, uint32_t*);
  void(MLN_TG_GL_API* delete_framebuffers)(int32_t, const uint32_t*);
  void(MLN_TG_GL_API* bind_framebuffer)(uint32_t, uint32_t);
  void(MLN_TG_GL_API* framebuffer_texture_2d)(
    uint32_t, uint32_t, uint32_t, uint32_t, int32_t
  );
  uint32_t(MLN_TG_GL_API* check_framebuffer_status)(uint32_t);
  void(MLN_TG_GL_API* read_pixels)(
    int32_t, int32_t, int32_t, int32_t, uint32_t, uint32_t, void*
  );
  void(MLN_TG_GL_API* finish)(void);
  uint32_t(MLN_TG_GL_API* get_error)(void);
} gl_functions;

typedef struct egl_functions {
  void* (*get_proc_address)(const char*);
  void* (*get_display)(void*);
  void* (*get_platform_display)(uint32_t, void*, const intptr_t*);
  unsigned (*initialize)(void*, int32_t*, int32_t*);
  unsigned (*choose_config)(void*, const int32_t*, void**, int32_t, int32_t*);
  unsigned (*bind_api)(uint32_t);
  void* (*create_context)(void*, void*, void*, const int32_t*);
  unsigned (*destroy_context)(void*, void*);
  void* (*create_pbuffer_surface)(void*, void*, const int32_t*);
  unsigned (*destroy_surface)(void*, void*);
  unsigned (*make_current)(void*, void*, void*, void*);
  void* (*get_current_context)(void);
  void* (*get_current_display)(void);
  void* (*get_current_surface)(int32_t);
  int32_t (*get_error)(void);
} egl_functions;

typedef struct metal_runtime {
  void* (*create_device)(void);
  void* (*get_class)(const char*);
  void* (*selector)(const char*);
  void (*message)(void);
  void* (*pool_push)(void);
  void (*pool_pop)(void*);
} metal_runtime;

#if defined(__ANDROID__)
// The NDK's AImageReader, which hands out an ANativeWindow with no view or
// activity behind it. Loaded from libmediandk.so, so the declarations here
// stand in for <media/NdkImageReader.h>.
typedef struct android_image_reader android_image_reader;
typedef struct android_image android_image;
typedef struct android_image_listener {
  void* context;
  void (*on_image_available)(void* context, android_image_reader* reader);
} android_image_listener;

typedef struct android_media_functions {
  int32_t (*new_with_usage)(
    int32_t width, int32_t height, int32_t format, uint64_t usage,
    int32_t max_images, android_image_reader** reader
  );
  int32_t (*get_window)(android_image_reader* reader, void** window);
  int32_t (*set_image_listener)(
    android_image_reader* reader, android_image_listener* listener
  );
  int32_t (*acquire_next_image)(
    android_image_reader* reader, android_image** image
  );
  void (*image_delete)(android_image* image);
  void (*reader_delete)(android_image_reader* reader);
} android_media_functions;
#endif

struct mln_test_graphics {
  mln_test_graphics_context context;
  void* library;
  metal_runtime objc;
  void* metal_queue;
  vulkan_functions vk;
  VkCommandPool vulkan_command_pool;
  bool vulkan_headless_surface;
  bool vulkan_metal_surface;
  bool vulkan_win32_surface;
  bool vulkan_android_surface;
#if defined(__ANDROID__)
  void* media_library;
  android_media_functions media;
#endif
  egl_functions egl;
  void* egl_pbuffer;
  void* gles_library;
  gl_functions gl;
  bool gl_loaded;
#if defined(MLN_TG_HAS_WGL)
  HWND window;
  int pixel_format;
  PIXELFORMATDESCRIPTOR pixel_format_descriptor;
  HGLRC(WINAPI* wgl_create_context)(HDC);
  BOOL(WINAPI* wgl_delete_context)(HGLRC);
  BOOL(WINAPI* wgl_make_current)(HDC, HGLRC);
  HGLRC(WINAPI* wgl_get_current_context)(void);
  HDC(WINAPI* wgl_get_current_dc)(void);
  PROC(WINAPI* wgl_get_proc_address)(LPCSTR);
#endif
};

struct mln_test_graphics_texture {
  mln_test_graphics* graphics;
  mln_test_graphics_texture_info info;
  VkDeviceMemory vulkan_memory;
};

struct mln_test_graphics_surface {
  mln_test_graphics* graphics;
  mln_test_graphics_surface_info info;
  void* vulkan_layer;
#if defined(MLN_TG_HAS_WGL)
  HWND window;
#endif
#if defined(__ANDROID__)
  android_image_reader* image_reader;
  android_image_listener image_listener;
#endif
};

// Metal, through the Objective-C runtime so the library stays plain C

#if defined(MLN_TG_HAS_METAL)

typedef struct metal_size {
  unsigned long width;
  unsigned long height;
  unsigned long depth;
} metal_size;

typedef struct metal_origin {
  unsigned long x;
  unsigned long y;
  unsigned long z;
} metal_origin;

typedef struct metal_cg_size {
  double width;
  double height;
} metal_cg_size;

typedef struct metal_cg_rect {
  double x;
  double y;
  double width;
  double height;
} metal_cg_rect;

enum {
  METAL_PIXEL_FORMAT_BGRA8_UNORM = 80,
  METAL_TEXTURE_TYPE_2D = 2,
  METAL_TEXTURE_USAGE_SHADER_READ = 1,
  METAL_TEXTURE_USAGE_RENDER_TARGET = 4,
  METAL_STORAGE_MODE_PRIVATE = 2,
  METAL_RESOURCE_STORAGE_MODE_SHARED = 0,
};

typedef void* (*message_object)(void*, void*);
typedef void* (*message_object_object)(void*, void*, void*);
typedef void (*message_void)(void*, void*);
typedef void (*message_set_object)(void*, void*, void*);
typedef void (*message_set_unsigned)(void*, void*, unsigned long);
typedef void (*message_set_size)(void*, void*, metal_cg_size);
typedef void (*message_set_rect)(void*, void*, metal_cg_rect);
typedef void (*message_set_double)(void*, void*, double);
typedef void* (*message_new_buffer)(void*, void*, unsigned long, unsigned long);
typedef void (*message_copy_texture_to_buffer)(
  void*, void*, void*, unsigned long, unsigned long, metal_origin, metal_size,
  void*, unsigned long, unsigned long, unsigned long
);

static void* objc_send(
  const mln_test_graphics* graphics, void* target, const char* selector
) {
  return ((message_object)graphics->objc.message)(
    target, graphics->objc.selector(selector)
  );
}

static void objc_send_void(
  const mln_test_graphics* graphics, void* target, const char* selector
) {
  ((message_void)graphics->objc.message)(
    target, graphics->objc.selector(selector)
  );
}

static void objc_set_unsigned(
  const mln_test_graphics* graphics, void* target, const char* selector,
  unsigned long value
) {
  ((message_set_unsigned)graphics->objc.message)(
    target, graphics->objc.selector(selector), value
  );
}

static void* objc_new(const mln_test_graphics* graphics, const char* name) {
  void* class_object = graphics->objc.get_class(name);
  if (class_object == NULL) return NULL;
  return objc_send(
    graphics, objc_send(graphics, class_object, "alloc"), "init"
  );
}

static bool metal_load(metal_runtime* objc) {
  static const char* const metal[] = {
    "/System/Library/Frameworks/Metal.framework/Metal"
  };
  static const char* const quartz[] = {
    "/System/Library/Frameworks/QuartzCore.framework/QuartzCore"
  };
  static const char* const runtime[] = {"/usr/lib/libobjc.A.dylib"};
  void* metal_library = library_open_first("Metal", metal, 1);
  if (metal_library == NULL) return false;
  // CAMetalLayer's class registers when QuartzCore loads.
  if (library_open_first("QuartzCore", quartz, 1) == NULL) return false;
  void* runtime_library =
    library_open_first("the Objective-C runtime", runtime, 1);
  if (runtime_library == NULL) return false;
  *(void**)&objc->create_device =
    library_symbol(metal_library, "MTLCreateSystemDefaultDevice");
  *(void**)&objc->get_class = library_symbol(runtime_library, "objc_getClass");
  *(void**)&objc->selector =
    library_symbol(runtime_library, "sel_registerName");
  *(void**)&objc->message = library_symbol(runtime_library, "objc_msgSend");
  *(void**)&objc->pool_push =
    library_symbol(runtime_library, "objc_autoreleasePoolPush");
  *(void**)&objc->pool_pop =
    library_symbol(runtime_library, "objc_autoreleasePoolPop");
  if (
    !objc->create_device || !objc->get_class || !objc->selector ||
    !objc->message || !objc->pool_push || !objc->pool_pop
  ) {
    set_error("the Metal or Objective-C runtime lacks an entry point");
    return false;
  }
  return true;
}

// Returns a CAMetalLayer with one reference that the caller owns. A layer in
// no window has no size of its own, and a Vulkan surface reports the layer's
// bounds as its extent, so the bounds carry the requested size too.
static void* metal_layer_create(
  const mln_test_graphics* graphics, void* device, uint32_t width,
  uint32_t height
) {
  void* layer = objc_new(graphics, "CAMetalLayer");
  if (layer == NULL) {
    set_error("could not create a CAMetalLayer");
    return NULL;
  }
  ((message_set_object)graphics->objc.message)(
    layer, graphics->objc.selector("setDevice:"), device
  );
  objc_set_unsigned(
    graphics, layer, "setPixelFormat:", METAL_PIXEL_FORMAT_BGRA8_UNORM
  );
  ((message_set_double)graphics->objc.message)(
    layer, graphics->objc.selector("setContentsScale:"), 1.0
  );
  ((message_set_rect)graphics->objc.message)(
    layer, graphics->objc.selector("setBounds:"),
    (metal_cg_rect){.width = width, .height = height}
  );
  ((message_set_size)graphics->objc.message)(
    layer, graphics->objc.selector("setDrawableSize:"),
    (metal_cg_size){.width = width, .height = height}
  );
  return layer;
}

static bool metal_create(mln_test_graphics* graphics) {
  if (!metal_load(&graphics->objc)) return false;
  graphics->context.metal_device = graphics->objc.create_device();
  if (graphics->context.metal_device == NULL) {
    set_error("this host has no Metal device");
    return false;
  }
  return true;
}

static void metal_destroy(mln_test_graphics* graphics) {
  if (graphics->metal_queue != NULL) {
    objc_send_void(graphics, graphics->metal_queue, "release");
  }
  if (graphics->context.metal_device != NULL) {
    objc_send_void(graphics, graphics->context.metal_device, "release");
  }
}

static bool metal_texture_create(
  mln_test_graphics* graphics, mln_test_graphics_texture* texture
) {
  void* pool = graphics->objc.pool_push();
  void* descriptor = objc_new(graphics, "MTLTextureDescriptor");
  if (descriptor != NULL) {
    objc_set_unsigned(
      graphics, descriptor, "setTextureType:", METAL_TEXTURE_TYPE_2D
    );
    objc_set_unsigned(
      graphics, descriptor, "setPixelFormat:", METAL_PIXEL_FORMAT_BGRA8_UNORM
    );
    objc_set_unsigned(graphics, descriptor, "setWidth:", texture->info.width);
    objc_set_unsigned(graphics, descriptor, "setHeight:", texture->info.height);
    objc_set_unsigned(
      graphics, descriptor, "setUsage:",
      METAL_TEXTURE_USAGE_RENDER_TARGET | METAL_TEXTURE_USAGE_SHADER_READ
    );
    objc_set_unsigned(
      graphics, descriptor, "setStorageMode:", METAL_STORAGE_MODE_PRIVATE
    );
    texture->info.metal_texture =
      ((message_object_object)graphics->objc.message)(
        graphics->context.metal_device,
        graphics->objc.selector("newTextureWithDescriptor:"), descriptor
      );
    objc_send_void(graphics, descriptor, "release");
  }
  graphics->objc.pool_pop(pool);
  if (texture->info.metal_texture == NULL) {
    set_error("could not create a Metal texture");
    return false;
  }
  texture->info.format = METAL_PIXEL_FORMAT_BGRA8_UNORM;
  return true;
}

static void metal_texture_destroy(mln_test_graphics_texture* texture) {
  if (texture->info.metal_texture != NULL) {
    objc_send_void(texture->graphics, texture->info.metal_texture, "release");
  }
}

static bool metal_texture_read(
  mln_test_graphics_texture* texture, uint8_t* pixels
) {
  mln_test_graphics* graphics = texture->graphics;
  const unsigned long width = texture->info.width;
  const unsigned long height = texture->info.height;
  if (graphics->metal_queue == NULL) {
    graphics->metal_queue =
      objc_send(graphics, graphics->context.metal_device, "newCommandQueue");
    if (graphics->metal_queue == NULL) {
      set_error("could not create a Metal command queue");
      return false;
    }
  }
  void* pool = graphics->objc.pool_push();
  void* buffer = ((message_new_buffer)graphics->objc.message)(
    graphics->context.metal_device,
    graphics->objc.selector("newBufferWithLength:options:"), width * height * 4,
    METAL_RESOURCE_STORAGE_MODE_SHARED
  );
  bool read = false;
  if (buffer != NULL) {
    void* commands =
      objc_send(graphics, graphics->metal_queue, "commandBuffer");
    void* blit = objc_send(graphics, commands, "blitCommandEncoder");
    ((message_copy_texture_to_buffer)graphics->objc.message)(
      blit,
      graphics->objc.selector(
        "copyFromTexture:sourceSlice:sourceLevel:sourceOrigin:sourceSize:"
        "toBuffer:destinationOffset:destinationBytesPerRow:"
        "destinationBytesPerImage:"
      ),
      texture->info.metal_texture, 0, 0, (metal_origin){0, 0, 0},
      (metal_size){width, height, 1}, buffer, 0, width * 4, width * height * 4
    );
    objc_send_void(graphics, blit, "endEncoding");
    objc_send_void(graphics, commands, "commit");
    objc_send_void(graphics, commands, "waitUntilCompleted");
    const uint8_t* bgra = objc_send(graphics, buffer, "contents");
    for (size_t index = 0; index < (size_t)width * height; index += 1) {
      pixels[index * 4 + 0] = bgra[index * 4 + 2];
      pixels[index * 4 + 1] = bgra[index * 4 + 1];
      pixels[index * 4 + 2] = bgra[index * 4 + 0];
      pixels[index * 4 + 3] = bgra[index * 4 + 3];
    }
    objc_send_void(graphics, buffer, "release");
    read = true;
  } else {
    set_error("could not create a Metal readback buffer");
  }
  graphics->objc.pool_pop(pool);
  return read;
}

static bool metal_surface_create(
  mln_test_graphics* graphics, mln_test_graphics_surface* surface
) {
  void* pool = graphics->objc.pool_push();
  surface->info.metal_layer = metal_layer_create(
    graphics, graphics->context.metal_device, surface->info.width,
    surface->info.height
  );
  graphics->objc.pool_pop(pool);
  return surface->info.metal_layer != NULL;
}

static void metal_surface_destroy(mln_test_graphics_surface* surface) {
  if (surface->info.metal_layer != NULL) {
    objc_send_void(surface->graphics, surface->info.metal_layer, "release");
  }
}

#endif

// Vulkan

static const char* vulkan_loader_name(void) {
#if defined(__APPLE__)
  return "libvulkan.1.dylib";
#elif defined(__ANDROID__) || defined(__OHOS__)
  // Neither platform's loader carries a versioned soname.
  return "libvulkan.so";
#elif defined(_WIN32)
  return "vulkan-1.dll";
#else
  return "libvulkan.so.1";
#endif
}

static void* vulkan_open_loader(void) {
  char from_environment[1024] = {0};
  const char* directory = getenv("MLN_FFI_VULKAN_LOADER_DIR");
  if (directory != NULL && directory[0] != '\0') {
    snprintf(
      from_environment, sizeof(from_environment), "%s/%s", directory,
      vulkan_loader_name()
    );
  }
  const char* const candidates[] = {
    from_environment,
#if defined(MLN_TEST_GRAPHICS_VULKAN_LOADER)
    MLN_TEST_GRAPHICS_VULKAN_LOADER,
#endif
    vulkan_loader_name(),
  };
  return library_open_first(
    "the Vulkan loader", candidates, sizeof(candidates) / sizeof(*candidates)
  );
}

static bool has_extension(
  const VkExtensionProperties* extensions, uint32_t count, const char* name
) {
  for (uint32_t index = 0; index < count; index += 1) {
    if (strcmp(extensions[index].extensionName, name) == 0) return true;
  }
  return false;
}

static bool vulkan_create_instance(mln_test_graphics* graphics) {
  vulkan_functions* vk = &graphics->vk;
  PFN_vkEnumerateInstanceExtensionProperties enumerate_extensions =
    (PFN_vkEnumerateInstanceExtensionProperties)vk->get_instance_proc_addr(
      NULL, "vkEnumerateInstanceExtensionProperties"
    );
  PFN_vkCreateInstance create_instance =
    (PFN_vkCreateInstance)vk->get_instance_proc_addr(NULL, "vkCreateInstance");
  if (enumerate_extensions == NULL || create_instance == NULL) {
    set_error("the Vulkan loader lacks vkCreateInstance");
    return false;
  }
  uint32_t available_count = 0;
  enumerate_extensions(NULL, &available_count, NULL);
  VkExtensionProperties* available =
    calloc(available_count + 1, sizeof(*available));
  if (available == NULL) {
    set_error("out of memory");
    return false;
  }
  enumerate_extensions(NULL, &available_count, available);

  // Surface extensions are optional: a host without them still gets texture
  // contexts, and mln_test_graphics_surface_create() reports the gap.
  const char* enabled[6];
  uint32_t enabled_count = 0;
  const bool portability =
    has_extension(available, available_count, "VK_KHR_portability_enumeration");
  if (portability) enabled[enabled_count++] = "VK_KHR_portability_enumeration";
  if (has_extension(available, available_count, "VK_KHR_surface")) {
    enabled[enabled_count++] = "VK_KHR_surface";
#if defined(__APPLE__)
    graphics->vulkan_metal_surface =
      has_extension(available, available_count, "VK_EXT_metal_surface");
    if (graphics->vulkan_metal_surface) {
      enabled[enabled_count++] = "VK_EXT_metal_surface";
    }
#elif defined(_WIN32)
    graphics->vulkan_win32_surface =
      has_extension(available, available_count, "VK_KHR_win32_surface");
    if (graphics->vulkan_win32_surface) {
      enabled[enabled_count++] = "VK_KHR_win32_surface";
    }
#elif defined(__ANDROID__)
    graphics->vulkan_android_surface =
      has_extension(available, available_count, "VK_KHR_android_surface");
    if (graphics->vulkan_android_surface) {
      enabled[enabled_count++] = "VK_KHR_android_surface";
    }
#endif
    graphics->vulkan_headless_surface =
      has_extension(available, available_count, "VK_EXT_headless_surface");
    if (graphics->vulkan_headless_surface) {
      enabled[enabled_count++] = "VK_EXT_headless_surface";
    }
  }
  free(available);

  const VkApplicationInfo application = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "mln_test_graphics",
    .apiVersion = VK_API_VERSION_1_1,
  };
  const VkInstanceCreateInfo instance_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .flags = portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0,
    .pApplicationInfo = &application,
    .enabledExtensionCount = enabled_count,
    .ppEnabledExtensionNames = enabled,
  };
  VkInstance instance = VK_NULL_HANDLE;
  const VkResult result = create_instance(&instance_info, NULL, &instance);
  if (result != VK_SUCCESS) {
    set_error("vkCreateInstance failed with %d", (int)result);
    return false;
  }
  graphics->context.vulkan_instance = instance;
  return true;
}

static PFN_vkVoidFunction vulkan_proc(
  const mln_test_graphics* graphics, const char* name
) {
  return graphics->vk.get_instance_proc_addr(
    (VkInstance)graphics->context.vulkan_instance, name
  );
}

static bool vulkan_load_functions(mln_test_graphics* graphics) {
  vulkan_functions* vk = &graphics->vk;
#define MLN_TG_LOAD(field, name)                                  \
  *(PFN_vkVoidFunction*)&vk->field = vulkan_proc(graphics, name); \
  if (vk->field == NULL) {                                        \
    set_error("the Vulkan loader lacks %s", name);                \
    return false;                                                 \
  }
  MLN_TG_LOAD(get_device_proc_addr, "vkGetDeviceProcAddr");
  MLN_TG_LOAD(destroy_instance, "vkDestroyInstance");
  MLN_TG_LOAD(get_memory_properties, "vkGetPhysicalDeviceMemoryProperties");
  MLN_TG_LOAD(destroy_device, "vkDestroyDevice");
  MLN_TG_LOAD(device_wait_idle, "vkDeviceWaitIdle");
  MLN_TG_LOAD(create_image, "vkCreateImage");
  MLN_TG_LOAD(destroy_image, "vkDestroyImage");
  MLN_TG_LOAD(get_image_memory_requirements, "vkGetImageMemoryRequirements");
  MLN_TG_LOAD(bind_image_memory, "vkBindImageMemory");
  MLN_TG_LOAD(create_image_view, "vkCreateImageView");
  MLN_TG_LOAD(destroy_image_view, "vkDestroyImageView");
  MLN_TG_LOAD(allocate_memory, "vkAllocateMemory");
  MLN_TG_LOAD(free_memory, "vkFreeMemory");
  MLN_TG_LOAD(map_memory, "vkMapMemory");
  MLN_TG_LOAD(unmap_memory, "vkUnmapMemory");
  MLN_TG_LOAD(create_buffer, "vkCreateBuffer");
  MLN_TG_LOAD(destroy_buffer, "vkDestroyBuffer");
  MLN_TG_LOAD(get_buffer_memory_requirements, "vkGetBufferMemoryRequirements");
  MLN_TG_LOAD(bind_buffer_memory, "vkBindBufferMemory");
  MLN_TG_LOAD(create_command_pool, "vkCreateCommandPool");
  MLN_TG_LOAD(destroy_command_pool, "vkDestroyCommandPool");
  MLN_TG_LOAD(allocate_command_buffers, "vkAllocateCommandBuffers");
  MLN_TG_LOAD(free_command_buffers, "vkFreeCommandBuffers");
  MLN_TG_LOAD(begin_command_buffer, "vkBeginCommandBuffer");
  MLN_TG_LOAD(end_command_buffer, "vkEndCommandBuffer");
  MLN_TG_LOAD(cmd_pipeline_barrier, "vkCmdPipelineBarrier");
  MLN_TG_LOAD(cmd_copy_image_to_buffer, "vkCmdCopyImageToBuffer");
  MLN_TG_LOAD(queue_submit, "vkQueueSubmit");
  MLN_TG_LOAD(queue_wait_idle, "vkQueueWaitIdle");
#undef MLN_TG_LOAD
  // Present only when the instance enabled the surface extensions.
  *(PFN_vkVoidFunction*)&vk->destroy_surface =
    vulkan_proc(graphics, "vkDestroySurfaceKHR");
  *(PFN_vkVoidFunction*)&vk->get_surface_support =
    vulkan_proc(graphics, "vkGetPhysicalDeviceSurfaceSupportKHR");
  *(PFN_vkVoidFunction*)&vk->create_headless_surface =
    vulkan_proc(graphics, "vkCreateHeadlessSurfaceEXT");
  vk->create_metal_surface = vulkan_proc(graphics, "vkCreateMetalSurfaceEXT");
  vk->create_win32_surface = vulkan_proc(graphics, "vkCreateWin32SurfaceKHR");
  vk->create_android_surface =
    vulkan_proc(graphics, "vkCreateAndroidSurfaceKHR");
  return true;
}

// Takes the first physical device and queue family that accept a graphics
// device, enabling VK_KHR_swapchain where the device has it.
static bool vulkan_create_device(mln_test_graphics* graphics) {
  VkInstance instance = (VkInstance)graphics->context.vulkan_instance;
  PFN_vkEnumeratePhysicalDevices enumerate_devices =
    (PFN_vkEnumeratePhysicalDevices)vulkan_proc(
      graphics, "vkEnumeratePhysicalDevices"
    );
  PFN_vkGetPhysicalDeviceQueueFamilyProperties get_queues =
    (PFN_vkGetPhysicalDeviceQueueFamilyProperties)vulkan_proc(
      graphics, "vkGetPhysicalDeviceQueueFamilyProperties"
    );
  PFN_vkGetPhysicalDeviceFeatures get_features =
    (PFN_vkGetPhysicalDeviceFeatures)vulkan_proc(
      graphics, "vkGetPhysicalDeviceFeatures"
    );
  PFN_vkEnumerateDeviceExtensionProperties enumerate_extensions =
    (PFN_vkEnumerateDeviceExtensionProperties)vulkan_proc(
      graphics, "vkEnumerateDeviceExtensionProperties"
    );
  PFN_vkCreateDevice create_device =
    (PFN_vkCreateDevice)vulkan_proc(graphics, "vkCreateDevice");
  PFN_vkGetDeviceQueue get_queue =
    (PFN_vkGetDeviceQueue)vulkan_proc(graphics, "vkGetDeviceQueue");
  if (
    !enumerate_devices || !get_queues || !get_features ||
    !enumerate_extensions || !create_device || !get_queue
  ) {
    set_error("the Vulkan loader lacks a device entry point");
    return false;
  }

  uint32_t device_count = 0;
  if (
    enumerate_devices(instance, &device_count, NULL) != VK_SUCCESS ||
    device_count == 0
  ) {
    set_error("the Vulkan instance has no physical device");
    return false;
  }
  VkPhysicalDevice* devices = calloc(device_count, sizeof(*devices));
  if (devices == NULL) {
    set_error("out of memory");
    return false;
  }
  enumerate_devices(instance, &device_count, devices);
  for (uint32_t d = 0; d < device_count && !graphics->context.vulkan_device;
       d += 1) {
    uint32_t queue_count = 0;
    get_queues(devices[d], &queue_count, NULL);
    VkQueueFamilyProperties* queues = calloc(queue_count + 1, sizeof(*queues));
    uint32_t extension_count = 0;
    enumerate_extensions(devices[d], NULL, &extension_count, NULL);
    VkExtensionProperties* extensions =
      calloc(extension_count + 1, sizeof(*extensions));
    if (queues == NULL || extensions == NULL) {
      free(queues);
      free(extensions);
      continue;
    }
    get_queues(devices[d], &queue_count, queues);
    enumerate_extensions(devices[d], NULL, &extension_count, extensions);
    const char* enabled[2];
    uint32_t enabled_count = 0;
    if (
      has_extension(extensions, extension_count, "VK_KHR_portability_subset")
    ) {
      enabled[enabled_count++] = "VK_KHR_portability_subset";
    }
    if (has_extension(extensions, extension_count, "VK_KHR_swapchain")) {
      enabled[enabled_count++] = "VK_KHR_swapchain";
    }
    free(extensions);
    VkPhysicalDeviceFeatures supported = {0};
    get_features(devices[d], &supported);
    const VkPhysicalDeviceFeatures features = {
      .samplerAnisotropy = supported.samplerAnisotropy,
      .wideLines = supported.wideLines,
    };
    for (uint32_t q = 0; q < queue_count; q += 1) {
      if (
        !(queues[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) ||
        queues[q].queueCount == 0
      ) {
        continue;
      }
      const float priority = 1.0F;
      const VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = q,
        .queueCount = 1,
        .pQueuePriorities = &priority,
      };
      const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = enabled_count,
        .ppEnabledExtensionNames = enabled,
        .pEnabledFeatures = &features,
      };
      VkDevice device = VK_NULL_HANDLE;
      if (
        create_device(devices[d], &device_info, NULL, &device) != VK_SUCCESS
      ) {
        continue;
      }
      VkQueue queue = VK_NULL_HANDLE;
      get_queue(device, q, 0, &queue);
      graphics->context.vulkan_physical_device = devices[d];
      graphics->context.vulkan_device = device;
      graphics->context.vulkan_queue = queue;
      graphics->context.vulkan_queue_family_index = q;
      break;
    }
    free(queues);
  }
  free(devices);
  if (graphics->context.vulkan_device == NULL) {
    set_error("no Vulkan physical device accepted a graphics device");
    return false;
  }
  return true;
}

static bool vulkan_create(mln_test_graphics* graphics) {
  graphics->library = vulkan_open_loader();
  if (graphics->library == NULL) return false;
  *(void**)&graphics->vk.get_instance_proc_addr =
    library_symbol(graphics->library, "vkGetInstanceProcAddr");
  if (graphics->vk.get_instance_proc_addr == NULL) {
    set_error("the Vulkan loader lacks vkGetInstanceProcAddr");
    return false;
  }
  if (
    !vulkan_create_instance(graphics) || !vulkan_load_functions(graphics) ||
    !vulkan_create_device(graphics)
  ) {
    return false;
  }
  graphics->context.vulkan_get_instance_proc_addr =
    (void*)(uintptr_t)graphics->vk.get_instance_proc_addr;
  graphics->context.vulkan_get_device_proc_addr =
    (void*)(uintptr_t)graphics->vk.get_device_proc_addr;
  return true;
}

static void vulkan_destroy(mln_test_graphics* graphics) {
  vulkan_functions* vk = &graphics->vk;
  VkDevice device = (VkDevice)graphics->context.vulkan_device;
  if (device != VK_NULL_HANDLE) {
    vk->device_wait_idle(device);
    if (graphics->vulkan_command_pool != VK_NULL_HANDLE) {
      vk->destroy_command_pool(device, graphics->vulkan_command_pool, NULL);
    }
    vk->destroy_device(device, NULL);
  }
  if (graphics->context.vulkan_instance != NULL && vk->destroy_instance) {
    vk->destroy_instance((VkInstance)graphics->context.vulkan_instance, NULL);
  }
}

static bool vulkan_memory_type(
  const mln_test_graphics* graphics, uint32_t type_bits,
  VkMemoryPropertyFlags properties, uint32_t* out_index
) {
  VkPhysicalDeviceMemoryProperties memory = {0};
  graphics->vk.get_memory_properties(
    (VkPhysicalDevice)graphics->context.vulkan_physical_device, &memory
  );
  for (uint32_t index = 0; index < memory.memoryTypeCount; index += 1) {
    if (
      (type_bits & (1U << index)) &&
      (memory.memoryTypes[index].propertyFlags & properties) == properties
    ) {
      *out_index = index;
      return true;
    }
  }
  set_error("no Vulkan memory type has the required properties");
  return false;
}

static bool vulkan_texture_create(
  mln_test_graphics* graphics, mln_test_graphics_texture* texture
) {
  const vulkan_functions* vk = &graphics->vk;
  VkDevice device = (VkDevice)graphics->context.vulkan_device;
  const VkImageCreateInfo image_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = VK_FORMAT_R8G8B8A8_UNORM,
    .extent = {texture->info.width, texture->info.height, 1},
    .mipLevels = 1,
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VkImage image = VK_NULL_HANDLE;
  if (vk->create_image(device, &image_info, NULL, &image) != VK_SUCCESS) {
    set_error("vkCreateImage failed");
    return false;
  }
  texture->info.vulkan_image = (uint64_t)(uintptr_t)image;
  VkMemoryRequirements requirements = {0};
  vk->get_image_memory_requirements(device, image, &requirements);
  uint32_t memory_type = 0;
  if (!vulkan_memory_type(
        graphics, requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_type
      )) {
    return false;
  }
  const VkMemoryAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = requirements.size,
    .memoryTypeIndex = memory_type,
  };
  if (
    vk->allocate_memory(
      device, &allocate_info, NULL, &texture->vulkan_memory
    ) != VK_SUCCESS ||
    vk->bind_image_memory(device, image, texture->vulkan_memory, 0) !=
      VK_SUCCESS
  ) {
    set_error("could not allocate and bind Vulkan image memory");
    return false;
  }
  const VkImageViewCreateInfo view_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .image = image,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = VK_FORMAT_R8G8B8A8_UNORM,
    .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkImageView view = VK_NULL_HANDLE;
  if (vk->create_image_view(device, &view_info, NULL, &view) != VK_SUCCESS) {
    set_error("vkCreateImageView failed");
    return false;
  }
  texture->info.vulkan_image_view = (uint64_t)(uintptr_t)view;
  texture->info.format = VK_FORMAT_R8G8B8A8_UNORM;
  texture->info.vulkan_initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
  texture->info.vulkan_final_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return true;
}

static void vulkan_texture_destroy(mln_test_graphics_texture* texture) {
  const vulkan_functions* vk = &texture->graphics->vk;
  VkDevice device = (VkDevice)texture->graphics->context.vulkan_device;
  vk->device_wait_idle(device);
  if (texture->info.vulkan_image_view != 0) {
    vk->destroy_image_view(
      device, (VkImageView)(uintptr_t)texture->info.vulkan_image_view, NULL
    );
  }
  if (texture->info.vulkan_image != 0) {
    vk->destroy_image(
      device, (VkImage)(uintptr_t)texture->info.vulkan_image, NULL
    );
  }
  if (texture->vulkan_memory != VK_NULL_HANDLE) {
    vk->free_memory(device, texture->vulkan_memory, NULL);
  }
}

static void vulkan_image_barrier(
  const vulkan_functions* vk, VkCommandBuffer commands, VkImage image,
  VkImageLayout from, VkImageLayout to, VkAccessFlags source_access,
  VkAccessFlags destination_access, VkPipelineStageFlags source_stage,
  VkPipelineStageFlags destination_stage
) {
  const VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = source_access,
    .dstAccessMask = destination_access,
    .oldLayout = from,
    .newLayout = to,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  vk->cmd_pipeline_barrier(
    commands, source_stage, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier
  );
}

// Copies the image through a host-visible buffer on the context's queue, and
// returns the image to its final layout afterwards.
static bool vulkan_texture_read(
  mln_test_graphics_texture* texture, uint8_t* pixels
) {
  mln_test_graphics* graphics = texture->graphics;
  const vulkan_functions* vk = &graphics->vk;
  VkDevice device = (VkDevice)graphics->context.vulkan_device;
  VkQueue queue = (VkQueue)graphics->context.vulkan_queue;
  VkImage image = (VkImage)(uintptr_t)texture->info.vulkan_image;
  const VkImageLayout final_layout = texture->info.vulkan_final_layout;
  const VkDeviceSize size =
    (VkDeviceSize)texture->info.width * texture->info.height * 4;

  if (graphics->vulkan_command_pool == VK_NULL_HANDLE) {
    const VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = graphics->context.vulkan_queue_family_index,
    };
    if (
      vk->create_command_pool(
        device, &pool_info, NULL, &graphics->vulkan_command_pool
      ) != VK_SUCCESS
    ) {
      set_error("vkCreateCommandPool failed");
      return false;
    }
  }

  bool read = false;
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkCommandBuffer commands = VK_NULL_HANDLE;
  const VkBufferCreateInfo buffer_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  if (vk->create_buffer(device, &buffer_info, NULL, &buffer) != VK_SUCCESS) {
    set_error("vkCreateBuffer failed");
    goto done;
  }
  VkMemoryRequirements requirements = {0};
  vk->get_buffer_memory_requirements(device, buffer, &requirements);
  uint32_t memory_type = 0;
  if (!vulkan_memory_type(
        graphics, requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &memory_type
      )) {
    goto done;
  }
  const VkMemoryAllocateInfo allocate_info = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = requirements.size,
    .memoryTypeIndex = memory_type,
  };
  if (
    vk->allocate_memory(device, &allocate_info, NULL, &memory) != VK_SUCCESS ||
    vk->bind_buffer_memory(device, buffer, memory, 0) != VK_SUCCESS
  ) {
    set_error("could not allocate and bind Vulkan readback memory");
    goto done;
  }
  const VkCommandBufferAllocateInfo commands_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = graphics->vulkan_command_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };
  if (
    vk->allocate_command_buffers(device, &commands_info, &commands) !=
    VK_SUCCESS
  ) {
    set_error("vkAllocateCommandBuffers failed");
    commands = VK_NULL_HANDLE;
    goto done;
  }
  const VkCommandBufferBeginInfo begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vk->begin_command_buffer(commands, &begin_info);
  vulkan_image_barrier(
    vk, commands, image, final_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
  );
  const VkBufferImageCopy region = {
    .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
    .imageExtent = {texture->info.width, texture->info.height, 1},
  };
  vk->cmd_copy_image_to_buffer(
    commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region
  );
  vulkan_image_barrier(
    vk, commands, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, final_layout,
    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
  );
  const VkBufferMemoryBarrier to_host = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .buffer = buffer,
    .size = VK_WHOLE_SIZE,
  };
  vk->cmd_pipeline_barrier(
    commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0,
    NULL, 1, &to_host, 0, NULL
  );
  if (vk->end_command_buffer(commands) != VK_SUCCESS) {
    set_error("vkEndCommandBuffer failed");
    goto done;
  }
  const VkSubmitInfo submit = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &commands,
  };
  if (
    vk->queue_submit(queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS ||
    vk->queue_wait_idle(queue) != VK_SUCCESS
  ) {
    set_error("the Vulkan readback submission failed");
    goto done;
  }
  void* mapped = NULL;
  if (vk->map_memory(device, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
    set_error("vkMapMemory failed");
    goto done;
  }
  memcpy(pixels, mapped, (size_t)size);
  vk->unmap_memory(device, memory);
  read = true;

done:
  if (commands != VK_NULL_HANDLE) {
    vk->free_command_buffers(
      device, graphics->vulkan_command_pool, 1, &commands
    );
  }
  if (buffer != VK_NULL_HANDLE) vk->destroy_buffer(device, buffer, NULL);
  if (memory != VK_NULL_HANDLE) vk->free_memory(device, memory, NULL);
  return read;
}

typedef struct vulkan_metal_surface_info {
  VkStructureType sType;
  const void* pNext;
  VkFlags flags;
  const void* pLayer;
} vulkan_metal_surface_info;

#if defined(_WIN32)
typedef struct vulkan_win32_surface_info {
  VkStructureType sType;
  const void* pNext;
  VkFlags flags;
  HINSTANCE hinstance;
  HWND hwnd;
} vulkan_win32_surface_info;
#endif

#if defined(MLN_TG_HAS_WGL)
static HWND wgl_create_window(uint32_t width, uint32_t height);
#endif

#if defined(__ANDROID__)
typedef struct vulkan_android_surface_info {
  VkStructureType sType;
  const void* pNext;
  VkFlags flags;
  void* window;
} vulkan_android_surface_info;

enum {
  android_image_format_rgba_8888 = 1,
  android_media_ok = 0,
};
// AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
// AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT
static const uint64_t android_gpu_usage = (1ULL << 8) | (1ULL << 9);

static bool android_media_load(mln_test_graphics* graphics) {
  if (graphics->media_library != NULL) return true;
  static const char* const names[] = {"libmediandk.so"};
  void* library = library_open_first("the NDK media library", names, 1);
  if (library == NULL) return false;
  android_media_functions* media = &graphics->media;
#define MLN_TG_LOAD(field, name)                          \
  *(void**)&media->field = library_symbol(library, name); \
  if (media->field == NULL) {                             \
    set_error("libmediandk.so lacks %s", name);           \
    return false;                                         \
  }
  MLN_TG_LOAD(new_with_usage, "AImageReader_newWithUsage");
  MLN_TG_LOAD(get_window, "AImageReader_getWindow");
  MLN_TG_LOAD(set_image_listener, "AImageReader_setImageListener");
  MLN_TG_LOAD(acquire_next_image, "AImageReader_acquireNextImage");
  MLN_TG_LOAD(image_delete, "AImage_delete");
  MLN_TG_LOAD(reader_delete, "AImageReader_delete");
#undef MLN_TG_LOAD
  graphics->media_library = library;
  return true;
}

// Consumes each presented image, so the swapchain never runs out of buffers to
// dequeue. The reader calls this on a thread of its own.
static void android_release_presented_image(
  void* context, android_image_reader* reader
) {
  const android_media_functions* media = context;
  android_image* image = NULL;
  if (media->acquire_next_image(reader, &image) == android_media_ok) {
    media->image_delete(image);
  }
}

// An image reader's window stands in for a view's: the swapchain presents
// into its buffer queue, and the listener above consumes each frame.
static void* android_window_create(
  mln_test_graphics* graphics, mln_test_graphics_surface* surface
) {
  if (!android_media_load(graphics)) return NULL;
  const android_media_functions* media = &graphics->media;
  int32_t status = media->new_with_usage(
    (int32_t)surface->info.width, (int32_t)surface->info.height,
    android_image_format_rgba_8888, android_gpu_usage, 4, &surface->image_reader
  );
  if (status != android_media_ok) {
    surface->image_reader = NULL;
    set_error("AImageReader_newWithUsage failed with %d", (int)status);
    return NULL;
  }
  surface->image_listener = (android_image_listener){
    .context = &graphics->media,
    .on_image_available = android_release_presented_image,
  };
  status =
    media->set_image_listener(surface->image_reader, &surface->image_listener);
  if (status != android_media_ok) {
    set_error("AImageReader_setImageListener failed with %d", (int)status);
    return NULL;
  }
  // The reader owns the window and releases it with the reader.
  void* window = NULL;
  status = media->get_window(surface->image_reader, &window);
  if (status != android_media_ok || window == NULL) {
    set_error("AImageReader_getWindow failed with %d", (int)status);
    return NULL;
  }
  return window;
}
#endif

// Prefers the platform's own surface kind, which a real window would use, and
// falls back to VK_EXT_headless_surface.
static bool vulkan_surface_create(
  mln_test_graphics* graphics, mln_test_graphics_surface* surface
) {
  const vulkan_functions* vk = &graphics->vk;
  VkInstance instance = (VkInstance)graphics->context.vulkan_instance;
  VkSurfaceKHR handle = VK_NULL_HANDLE;
  VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
#if defined(MLN_TG_HAS_METAL)
  if (graphics->vulkan_metal_surface && vk->create_metal_surface != NULL) {
    if (!metal_load(&graphics->objc)) return false;
    void* device = graphics->objc.create_device();
    void* pool = graphics->objc.pool_push();
    surface->vulkan_layer = metal_layer_create(
      graphics, device, surface->info.width, surface->info.height
    );
    graphics->objc.pool_pop(pool);
    if (device != NULL) objc_send_void(graphics, device, "release");
    if (surface->vulkan_layer == NULL) return false;
    const vulkan_metal_surface_info info = {
      .sType = (VkStructureType)1000217000,
      .pLayer = surface->vulkan_layer,
    };
    result = ((VkResult(VKAPI_PTR*)(
      VkInstance, const vulkan_metal_surface_info*,
      const VkAllocationCallbacks*, VkSurfaceKHR*
    ))vk->create_metal_surface)(instance, &info, NULL, &handle);
  }
#endif
#if defined(_WIN32)
  if (
    handle == VK_NULL_HANDLE && graphics->vulkan_win32_surface &&
    vk->create_win32_surface != NULL
  ) {
    surface->window =
      wgl_create_window(surface->info.width, surface->info.height);
    if (surface->window == NULL) return false;
    const vulkan_win32_surface_info info = {
      .sType = (VkStructureType)1000009000,
      .hinstance = GetModuleHandleA(NULL),
      .hwnd = surface->window,
    };
    result = ((VkResult(VKAPI_PTR*)(
      VkInstance, const vulkan_win32_surface_info*,
      const VkAllocationCallbacks*, VkSurfaceKHR*
    ))vk->create_win32_surface)(instance, &info, NULL, &handle);
  }
#endif
#if defined(__ANDROID__)
  if (
    handle == VK_NULL_HANDLE && graphics->vulkan_android_surface &&
    vk->create_android_surface != NULL
  ) {
    void* window = android_window_create(graphics, surface);
    if (window == NULL) return false;
    const vulkan_android_surface_info info = {
      .sType = (VkStructureType)1000008000,
      .window = window,
    };
    result = ((VkResult(VKAPI_PTR*)(
      VkInstance, const vulkan_android_surface_info*,
      const VkAllocationCallbacks*, VkSurfaceKHR*
    ))vk->create_android_surface)(instance, &info, NULL, &handle);
  }
#endif
  if (
    handle == VK_NULL_HANDLE && graphics->vulkan_headless_surface &&
    vk->create_headless_surface != NULL
  ) {
    const VkHeadlessSurfaceCreateInfoEXT info = {
      .sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT,
    };
    result = vk->create_headless_surface(instance, &info, NULL, &handle);
  }
  if (handle == VK_NULL_HANDLE) {
    set_error(
      "this Vulkan instance has no surface extension to create a surface "
      "with (%d)",
      (int)result
    );
    return false;
  }
  surface->info.vulkan_surface = (uint64_t)(uintptr_t)handle;
  VkBool32 supported = VK_FALSE;
  if (
    vk->get_surface_support == NULL ||
    vk->get_surface_support(
      (VkPhysicalDevice)graphics->context.vulkan_physical_device,
      graphics->context.vulkan_queue_family_index, handle, &supported
    ) != VK_SUCCESS ||
    !supported
  ) {
    set_error("the context's queue family cannot present to the surface");
    return false;
  }
  return true;
}

static void vulkan_surface_destroy(mln_test_graphics_surface* surface) {
  mln_test_graphics* graphics = surface->graphics;
  if (surface->info.vulkan_surface != 0 && graphics->vk.destroy_surface) {
    graphics->vk.destroy_surface(
      (VkInstance)graphics->context.vulkan_instance,
      (VkSurfaceKHR)(uintptr_t)surface->info.vulkan_surface, NULL
    );
  }
#if defined(MLN_TG_HAS_METAL)
  if (surface->vulkan_layer != NULL) {
    objc_send_void(graphics, surface->vulkan_layer, "release");
  }
#endif
#if defined(__ANDROID__)
  // After the surface, which holds the reader's window.
  if (surface->image_reader != NULL) {
    graphics->media.reader_delete(surface->image_reader);
  }
#endif
}

// OpenGL objects, shared by EGL and WGL

enum {
  GL_TEXTURE_2D_VALUE = 0x0DE1,
  GL_RGBA8_VALUE = 0x8058,
  GL_RGBA_VALUE = 0x1908,
  GL_UNSIGNED_BYTE_VALUE = 0x1401,
  GL_TEXTURE_MAG_FILTER_VALUE = 0x2800,
  GL_TEXTURE_MIN_FILTER_VALUE = 0x2801,
  GL_LINEAR_VALUE = 0x2601,
  GL_FRAMEBUFFER_VALUE = 0x8D40,
  GL_COLOR_ATTACHMENT0_VALUE = 0x8CE0,
  GL_FRAMEBUFFER_COMPLETE_VALUE = 0x8CD5,
};

#if defined(MLN_TG_HAS_GL)

// The context switch a GL call needs, which puts back whatever the calling
// thread had current before.
typedef struct gl_current {
  void* display;
  void* draw;
  void* read;
  void* context;
} gl_current;

static bool gl_enter(mln_test_graphics* graphics, gl_current* saved);
static void gl_leave(mln_test_graphics* graphics, const gl_current* saved);
static void* gl_symbol(mln_test_graphics* graphics, const char* name);

static bool gl_load(mln_test_graphics* graphics) {
  if (graphics->gl_loaded) return true;
  gl_functions* gl = &graphics->gl;
#define MLN_TG_LOAD(field, name)                   \
  *(void**)&gl->field = gl_symbol(graphics, name); \
  if (gl->field == NULL) {                         \
    set_error("OpenGL lacks %s", name);            \
    return false;                                  \
  }
  MLN_TG_LOAD(gen_textures, "glGenTextures");
  MLN_TG_LOAD(delete_textures, "glDeleteTextures");
  MLN_TG_LOAD(bind_texture, "glBindTexture");
  MLN_TG_LOAD(tex_image_2d, "glTexImage2D");
  MLN_TG_LOAD(tex_parameteri, "glTexParameteri");
  MLN_TG_LOAD(gen_framebuffers, "glGenFramebuffers");
  MLN_TG_LOAD(delete_framebuffers, "glDeleteFramebuffers");
  MLN_TG_LOAD(bind_framebuffer, "glBindFramebuffer");
  MLN_TG_LOAD(framebuffer_texture_2d, "glFramebufferTexture2D");
  MLN_TG_LOAD(check_framebuffer_status, "glCheckFramebufferStatus");
  MLN_TG_LOAD(read_pixels, "glReadPixels");
  MLN_TG_LOAD(finish, "glFinish");
  MLN_TG_LOAD(get_error, "glGetError");
#undef MLN_TG_LOAD
  graphics->gl_loaded = true;
  return true;
}

static bool gl_texture_create(
  mln_test_graphics* graphics, mln_test_graphics_texture* texture
) {
  gl_current saved;
  if (!gl_enter(graphics, &saved)) return false;
  bool created = false;
  if (gl_load(graphics)) {
    const gl_functions* gl = &graphics->gl;
    uint32_t name = 0;
    gl->gen_textures(1, &name);
    gl->bind_texture(GL_TEXTURE_2D_VALUE, name);
    gl->tex_parameteri(
      GL_TEXTURE_2D_VALUE, GL_TEXTURE_MIN_FILTER_VALUE, GL_LINEAR_VALUE
    );
    gl->tex_parameteri(
      GL_TEXTURE_2D_VALUE, GL_TEXTURE_MAG_FILTER_VALUE, GL_LINEAR_VALUE
    );
    gl->tex_image_2d(
      GL_TEXTURE_2D_VALUE, 0, GL_RGBA8_VALUE, (int32_t)texture->info.width,
      (int32_t)texture->info.height, 0, GL_RGBA_VALUE, GL_UNSIGNED_BYTE_VALUE,
      NULL
    );
    gl->bind_texture(GL_TEXTURE_2D_VALUE, 0);
    // The session context samples the texture from another context, which
    // sees it only once this context's commands have executed.
    gl->finish();
    const uint32_t error = gl->get_error();
    if (name == 0 || error != 0) {
      set_error("could not create an OpenGL texture (0x%x)", error);
      if (name != 0) gl->delete_textures(1, &name);
    } else {
      texture->info.opengl_texture = name;
      texture->info.opengl_target = GL_TEXTURE_2D_VALUE;
      texture->info.format = GL_RGBA8_VALUE;
      created = true;
    }
  }
  gl_leave(graphics, &saved);
  return created;
}

static void gl_texture_destroy(mln_test_graphics_texture* texture) {
  if (texture->info.opengl_texture == 0) return;
  gl_current saved;
  if (!gl_enter(texture->graphics, &saved)) return;
  texture->graphics->gl.delete_textures(1, &texture->info.opengl_texture);
  gl_leave(texture->graphics, &saved);
}

static bool gl_texture_read(
  mln_test_graphics_texture* texture, uint8_t* pixels
) {
  mln_test_graphics* graphics = texture->graphics;
  gl_current saved;
  if (!gl_enter(graphics, &saved)) return false;
  const gl_functions* gl = &graphics->gl;
  uint32_t framebuffer = 0;
  gl->gen_framebuffers(1, &framebuffer);
  gl->bind_framebuffer(GL_FRAMEBUFFER_VALUE, framebuffer);
  gl->framebuffer_texture_2d(
    GL_FRAMEBUFFER_VALUE, GL_COLOR_ATTACHMENT0_VALUE, GL_TEXTURE_2D_VALUE,
    texture->info.opengl_texture, 0
  );
  bool read = false;
  const uint32_t status = gl->check_framebuffer_status(GL_FRAMEBUFFER_VALUE);
  if (status == GL_FRAMEBUFFER_COMPLETE_VALUE) {
    gl->read_pixels(
      0, 0, (int32_t)texture->info.width, (int32_t)texture->info.height,
      GL_RGBA_VALUE, GL_UNSIGNED_BYTE_VALUE, pixels
    );
    const uint32_t error = gl->get_error();
    read = error == 0;
    if (!read) set_error("glReadPixels failed (0x%x)", error);
  } else {
    set_error("the texture's framebuffer is incomplete (0x%x)", status);
  }
  gl->bind_framebuffer(GL_FRAMEBUFFER_VALUE, 0);
  gl->delete_framebuffers(1, &framebuffer);
  gl_leave(graphics, &saved);
  return read;
}

#endif

// EGL

#if defined(MLN_TG_HAS_EGL)

enum {
  EGL_NONE_VALUE = 0x3038,
  EGL_SURFACE_TYPE_VALUE = 0x3033,
  EGL_PBUFFER_BIT_VALUE = 0x0001,
  EGL_RENDERABLE_TYPE_VALUE = 0x3040,
  EGL_OPENGL_ES3_BIT_VALUE = 0x0040,
  EGL_RED_SIZE_VALUE = 0x3024,
  EGL_GREEN_SIZE_VALUE = 0x3023,
  EGL_BLUE_SIZE_VALUE = 0x3022,
  EGL_ALPHA_SIZE_VALUE = 0x3021,
  EGL_DEPTH_SIZE_VALUE = 0x3025,
  EGL_STENCIL_SIZE_VALUE = 0x3026,
  EGL_WIDTH_VALUE = 0x3057,
  EGL_HEIGHT_VALUE = 0x3056,
  EGL_OPENGL_ES_API_VALUE = 0x30A0,
  EGL_CONTEXT_CLIENT_VERSION_VALUE = 0x3098,
  EGL_DRAW_VALUE = 0x3059,
  EGL_READ_VALUE = 0x305A,
  EGL_PLATFORM_ANGLE_ANGLE_VALUE = 0x3202,
  EGL_PLATFORM_ANGLE_TYPE_ANGLE_VALUE = 0x3203,
  EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE_VALUE = 0x3489,
  EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE_VALUE = 0x3209,
  EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE_VALUE = 0x320A,
  EGL_PLATFORM_SURFACELESS_MESA_VALUE = 0x31DD,
};

static void* egl_open_library(
  const char* what, const char* configured, const char* const* names,
  size_t count
) {
  const char* candidates[4] = {configured};
  for (size_t index = 0; index < count && index < 3; index += 1) {
    candidates[index + 1] = names[index];
  }
  return library_open_first(what, candidates, count + 1);
}

static bool egl_load(mln_test_graphics* graphics) {
#if defined(__APPLE__)
  static const char* const egl_names[] = {"libEGL.dylib"};
  static const char* const gles_names[] = {"libGLESv2.dylib"};
#elif defined(__ANDROID__)
  static const char* const egl_names[] = {"libEGL.so"};
  static const char* const gles_names[] = {"libGLESv3.so", "libGLESv2.so"};
#else
  static const char* const egl_names[] = {"libEGL.so.1", "libEGL.so"};
  static const char* const gles_names[] = {
    "libGLESv2.so.2", "libGLESv2.so", "libGLESv3.so"
  };
#endif
#if defined(MLN_TEST_GRAPHICS_EGL_LIBRARY)
  const char* configured_egl = MLN_TEST_GRAPHICS_EGL_LIBRARY;
#else
  const char* configured_egl = NULL;
#endif
#if defined(MLN_TEST_GRAPHICS_GLES_LIBRARY)
  const char* configured_gles = MLN_TEST_GRAPHICS_GLES_LIBRARY;
#else
  const char* configured_gles = NULL;
#endif
  graphics->library = egl_open_library(
    "EGL", configured_egl, egl_names, sizeof(egl_names) / sizeof(*egl_names)
  );
  if (graphics->library == NULL) return false;
  // Optional: eglGetProcAddress resolves core entry points on most drivers,
  // and this library answers for the ones it does not.
  graphics->gles_library = egl_open_library(
    "OpenGL ES", configured_gles, gles_names,
    sizeof(gles_names) / sizeof(*gles_names)
  );
  egl_functions* egl = &graphics->egl;
#define MLN_TG_LOAD(field, name)                                  \
  *(void**)&egl->field = library_symbol(graphics->library, name); \
  if (egl->field == NULL) {                                       \
    set_error("EGL lacks %s", name);                              \
    return false;                                                 \
  }
  MLN_TG_LOAD(get_proc_address, "eglGetProcAddress");
  MLN_TG_LOAD(get_display, "eglGetDisplay");
  MLN_TG_LOAD(initialize, "eglInitialize");
  MLN_TG_LOAD(choose_config, "eglChooseConfig");
  MLN_TG_LOAD(bind_api, "eglBindAPI");
  MLN_TG_LOAD(create_context, "eglCreateContext");
  MLN_TG_LOAD(destroy_context, "eglDestroyContext");
  MLN_TG_LOAD(create_pbuffer_surface, "eglCreatePbufferSurface");
  MLN_TG_LOAD(destroy_surface, "eglDestroySurface");
  MLN_TG_LOAD(make_current, "eglMakeCurrent");
  MLN_TG_LOAD(get_current_context, "eglGetCurrentContext");
  MLN_TG_LOAD(get_current_display, "eglGetCurrentDisplay");
  MLN_TG_LOAD(get_current_surface, "eglGetCurrentSurface");
  MLN_TG_LOAD(get_error, "eglGetError");
#undef MLN_TG_LOAD
  *(void**)&egl->get_platform_display =
    library_symbol(graphics->library, "eglGetPlatformDisplay");
  return true;
}

// Test contexts render into pbuffers and never present, so desktop Linux names
// the surfaceless platform: the default display resolves to whatever libEGL was
// built for, commonly X11, which fails without a display server. Apple uses
// ANGLE on Metal. Android and OpenHarmony keep the default display, which
// serves their own window systems.
static void* egl_display(const mln_test_graphics* graphics) {
  const egl_functions* egl = &graphics->egl;
#if defined(__ANDROID__) || defined(__OHOS__)
  return egl->get_display(NULL);
#else
  void* (*get_platform_display)(uint32_t, void*, const intptr_t*) =
    egl->get_platform_display;
  if (get_platform_display == NULL) return NULL;
#if defined(__APPLE__)
  const intptr_t attributes[] = {
    EGL_PLATFORM_ANGLE_TYPE_ANGLE_VALUE,
    EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE_VALUE,
    EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE_VALUE,
    EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE_VALUE,
    EGL_NONE_VALUE,
  };
  return get_platform_display(EGL_PLATFORM_ANGLE_ANGLE_VALUE, NULL, attributes);
#else
  return get_platform_display(EGL_PLATFORM_SURFACELESS_MESA_VALUE, NULL, NULL);
#endif
#endif
}

static bool egl_create(mln_test_graphics* graphics) {
  if (!egl_load(graphics)) return false;
  const egl_functions* egl = &graphics->egl;
  void* display = egl_display(graphics);
  if (display == NULL || !egl->initialize(display, NULL, NULL)) {
    set_error("could not initialize an EGL display (0x%x)", egl->get_error());
    return false;
  }
  graphics->context.egl_display = display;
  const int32_t config_attributes[] = {
    EGL_SURFACE_TYPE_VALUE,
    EGL_PBUFFER_BIT_VALUE,
    EGL_RENDERABLE_TYPE_VALUE,
    EGL_OPENGL_ES3_BIT_VALUE,
    EGL_RED_SIZE_VALUE,
    8,
    EGL_GREEN_SIZE_VALUE,
    8,
    EGL_BLUE_SIZE_VALUE,
    8,
    EGL_ALPHA_SIZE_VALUE,
    8,
    EGL_DEPTH_SIZE_VALUE,
    24,
    EGL_STENCIL_SIZE_VALUE,
    8,
    EGL_NONE_VALUE,
  };
  int32_t count = 0;
  if (
    !egl->choose_config(
      display, config_attributes, &graphics->context.egl_config, 1, &count
    ) ||
    count == 0 || graphics->context.egl_config == NULL
  ) {
    set_error("no EGL config supports OpenGL ES 3 pbuffers");
    return false;
  }
  if (!egl->bind_api(EGL_OPENGL_ES_API_VALUE)) {
    set_error("could not bind the OpenGL ES API");
    return false;
  }
  const int32_t context_attributes[] = {
    EGL_CONTEXT_CLIENT_VERSION_VALUE, 3, EGL_NONE_VALUE
  };
  graphics->context.egl_context = egl->create_context(
    display, graphics->context.egl_config, NULL, context_attributes
  );
  const int32_t pbuffer_attributes[] = {
    EGL_WIDTH_VALUE, 8, EGL_HEIGHT_VALUE, 8, EGL_NONE_VALUE
  };
  graphics->egl_pbuffer = egl->create_pbuffer_surface(
    display, graphics->context.egl_config, pbuffer_attributes
  );
  if (graphics->context.egl_context == NULL || graphics->egl_pbuffer == NULL) {
    set_error("could not create an EGL context (0x%x)", egl->get_error());
    return false;
  }
  graphics->context.get_proc_address = (void*)(uintptr_t)egl->get_proc_address;
  return true;
}

// The display stays initialized: eglTerminate() acts on every user of the
// display in the process, including sessions and other graphics objects that
// still hold objects on it.
static void egl_destroy(mln_test_graphics* graphics) {
  const egl_functions* egl = &graphics->egl;
  void* display = graphics->context.egl_display;
  if (display == NULL) return;
  if (
    graphics->context.egl_context != NULL &&
    egl->get_current_context() == graphics->context.egl_context
  ) {
    egl->make_current(display, NULL, NULL, NULL);
  }
  if (graphics->egl_pbuffer != NULL) {
    egl->destroy_surface(display, graphics->egl_pbuffer);
  }
  if (graphics->context.egl_context != NULL) {
    egl->destroy_context(display, graphics->context.egl_context);
  }
}

static bool egl_make_current(mln_test_graphics* graphics) {
  const egl_functions* egl = &graphics->egl;
  if (
    !egl->bind_api(EGL_OPENGL_ES_API_VALUE) ||
    !egl->make_current(
      graphics->context.egl_display, graphics->egl_pbuffer,
      graphics->egl_pbuffer, graphics->context.egl_context
    )
  ) {
    set_error("eglMakeCurrent failed (0x%x)", egl->get_error());
    return false;
  }
  return true;
}

static bool egl_surface_create(
  mln_test_graphics* graphics, mln_test_graphics_surface* surface
) {
  const int32_t attributes[] = {
    EGL_WIDTH_VALUE,  (int32_t)surface->info.width,
    EGL_HEIGHT_VALUE, (int32_t)surface->info.height,
    EGL_NONE_VALUE,
  };
  surface->info.opengl_surface = graphics->egl.create_pbuffer_surface(
    graphics->context.egl_display, graphics->context.egl_config, attributes
  );
  if (surface->info.opengl_surface == NULL) {
    set_error(
      "could not create an EGL pbuffer (0x%x)", graphics->egl.get_error()
    );
    return false;
  }
  return true;
}

static void egl_surface_destroy(mln_test_graphics_surface* surface) {
  if (surface->info.opengl_surface != NULL) {
    surface->graphics->egl.destroy_surface(
      surface->graphics->context.egl_display, surface->info.opengl_surface
    );
  }
}

#endif

// WGL

#if defined(MLN_TG_HAS_WGL)

static const char wgl_window_class[] = "MlnTestGraphicsWindow";

static HWND wgl_create_window(uint32_t width, uint32_t height) {
  const HINSTANCE instance = GetModuleHandleA(NULL);
  const WNDCLASSA window_class = {
    .style = CS_OWNDC,
    .lpfnWndProc = DefWindowProcA,
    .hInstance = instance,
    .lpszClassName = wgl_window_class,
  };
  // A second registration fails with ERROR_CLASS_ALREADY_EXISTS, which leaves
  // the first one in place.
  RegisterClassA(&window_class);
  // A pop-up window has no frame, so its client area, which is what a surface
  // on it reports as its extent, is the requested size.
  HWND window = CreateWindowExA(
    0, wgl_window_class, wgl_window_class, WS_POPUP, 0, 0, (int)width,
    (int)height, NULL, NULL, instance, NULL
  );
  if (window == NULL) {
    set_error("CreateWindowExA failed (%lu)", GetLastError());
  }
  return window;
}

static bool wgl_load(mln_test_graphics* graphics) {
  static const char* const names[] = {"opengl32.dll"};
  graphics->library = library_open_first("OpenGL", names, 1);
  if (graphics->library == NULL) return false;
#define MLN_TG_LOAD(field, name)                                       \
  *(void**)&graphics->field = library_symbol(graphics->library, name); \
  if (graphics->field == NULL) {                                       \
    set_error("opengl32 lacks %s", name);                              \
    return false;                                                      \
  }
  MLN_TG_LOAD(wgl_create_context, "wglCreateContext");
  MLN_TG_LOAD(wgl_delete_context, "wglDeleteContext");
  MLN_TG_LOAD(wgl_make_current, "wglMakeCurrent");
  MLN_TG_LOAD(wgl_get_current_context, "wglGetCurrentContext");
  MLN_TG_LOAD(wgl_get_current_dc, "wglGetCurrentDC");
  MLN_TG_LOAD(wgl_get_proc_address, "wglGetProcAddress");
#undef MLN_TG_LOAD
  return true;
}

static bool wgl_create(mln_test_graphics* graphics) {
  if (!wgl_load(graphics)) return false;
  graphics->window = wgl_create_window(8, 8);
  if (graphics->window == NULL) return false;
  HDC device_context = GetDC(graphics->window);
  graphics->context.wgl_device_context = device_context;
  graphics->pixel_format_descriptor = (PIXELFORMATDESCRIPTOR){
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    .iPixelType = PFD_TYPE_RGBA,
    .cColorBits = 32,
    .cAlphaBits = 8,
    .cDepthBits = 24,
    .cStencilBits = 8,
    .iLayerType = PFD_MAIN_PLANE,
  };
  graphics->pixel_format =
    ChoosePixelFormat(device_context, &graphics->pixel_format_descriptor);
  if (
    graphics->pixel_format == 0 ||
    !SetPixelFormat(
      device_context, graphics->pixel_format, &graphics->pixel_format_descriptor
    )
  ) {
    set_error("could not set a WGL pixel format (%lu)", GetLastError());
    return false;
  }
  HGLRC context = graphics->wgl_create_context(device_context);
  if (context == NULL) {
    set_error("wglCreateContext failed (%lu)", GetLastError());
    return false;
  }
  graphics->context.wgl_context = context;
  graphics->context.get_proc_address =
    (void*)(uintptr_t)graphics->wgl_get_proc_address;
  return true;
}

static void wgl_destroy(mln_test_graphics* graphics) {
  if (graphics->context.wgl_context != NULL) {
    if (graphics->wgl_get_current_context() == graphics->context.wgl_context) {
      graphics->wgl_make_current(NULL, NULL);
    }
    graphics->wgl_delete_context(graphics->context.wgl_context);
  }
  if (graphics->window != NULL) {
    if (graphics->context.wgl_device_context != NULL) {
      ReleaseDC(graphics->window, graphics->context.wgl_device_context);
    }
    DestroyWindow(graphics->window);
  }
}

static bool wgl_make_current(mln_test_graphics* graphics) {
  if (!graphics->wgl_make_current(
        graphics->context.wgl_device_context, graphics->context.wgl_context
      )) {
    set_error("wglMakeCurrent failed (%lu)", GetLastError());
    return false;
  }
  return true;
}

static bool wgl_surface_create(
  mln_test_graphics* graphics, mln_test_graphics_surface* surface
) {
  surface->window =
    wgl_create_window(surface->info.width, surface->info.height);
  if (surface->window == NULL) return false;
  HDC device_context = GetDC(surface->window);
  surface->info.opengl_surface = device_context;
  if (!SetPixelFormat(
        device_context, graphics->pixel_format,
        &graphics->pixel_format_descriptor
      )) {
    set_error("could not set the surface's pixel format (%lu)", GetLastError());
    return false;
  }
  return true;
}

static void wgl_surface_destroy(mln_test_graphics_surface* surface) {
  if (surface->window == NULL) return;
  if (surface->info.opengl_surface != NULL) {
    ReleaseDC(surface->window, surface->info.opengl_surface);
  }
  DestroyWindow(surface->window);
}

#endif

#if defined(MLN_TG_HAS_GL)

static bool gl_enter(mln_test_graphics* graphics, gl_current* saved) {
  *saved = (gl_current){0};
#if defined(MLN_TG_HAS_EGL)
  if (graphics->context.backend == MLN_TEST_GRAPHICS_BACKEND_EGL) {
    const egl_functions* egl = &graphics->egl;
    saved->display = egl->get_current_display();
    saved->context = egl->get_current_context();
    saved->draw = egl->get_current_surface(EGL_DRAW_VALUE);
    saved->read = egl->get_current_surface(EGL_READ_VALUE);
    return egl_make_current(graphics);
  }
#endif
#if defined(MLN_TG_HAS_WGL)
  if (graphics->context.backend == MLN_TEST_GRAPHICS_BACKEND_WGL) {
    saved->display = graphics->wgl_get_current_dc();
    saved->context = graphics->wgl_get_current_context();
    return wgl_make_current(graphics);
  }
#endif
  set_error("the graphics object has no OpenGL context");
  return false;
}

static void gl_leave(mln_test_graphics* graphics, const gl_current* saved) {
#if defined(MLN_TG_HAS_EGL)
  if (graphics->context.backend == MLN_TEST_GRAPHICS_BACKEND_EGL) {
    const egl_functions* egl = &graphics->egl;
    if (saved->display != NULL) {
      egl->make_current(
        saved->display, saved->draw, saved->read, saved->context
      );
    } else {
      egl->make_current(graphics->context.egl_display, NULL, NULL, NULL);
    }
    return;
  }
#endif
#if defined(MLN_TG_HAS_WGL)
  graphics->wgl_make_current(saved->display, saved->context);
#endif
  (void)graphics;
  (void)saved;
}

static void* gl_symbol(mln_test_graphics* graphics, const char* name) {
  void* symbol = NULL;
#if defined(MLN_TG_HAS_EGL)
  if (graphics->context.backend == MLN_TEST_GRAPHICS_BACKEND_EGL) {
    symbol = graphics->egl.get_proc_address(name);
    if (symbol == NULL) symbol = library_symbol(graphics->gles_library, name);
  }
#endif
#if defined(MLN_TG_HAS_WGL)
  if (graphics->context.backend == MLN_TEST_GRAPHICS_BACKEND_WGL) {
    // wglGetProcAddress answers only for entry points past OpenGL 1.1, and
    // some drivers return small sentinel values instead of null for the rest.
    symbol = (void*)(uintptr_t)graphics->wgl_get_proc_address(name);
    if ((uintptr_t)symbol <= 3 || (intptr_t)symbol == -1) {
      symbol = library_symbol(graphics->library, name);
    }
  }
#endif
  return symbol;
}

#endif

// The public API

static const char* backend_name(uint32_t backend) {
  switch (backend) {
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      return "Metal";
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      return "Vulkan";
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
      return "EGL";
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      return "WGL";
    default:
      return "an unknown backend";
  }
}

mln_test_graphics* mln_test_graphics_create(uint32_t backend) {
  mln_test_graphics* graphics = calloc(1, sizeof(*graphics));
  if (graphics == NULL) {
    set_error("out of memory");
    return NULL;
  }
  graphics->context.backend = backend;
  bool created = false;
  switch (backend) {
#if defined(MLN_TG_HAS_METAL)
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      created = metal_create(graphics);
      break;
#endif
#if !defined(__EMSCRIPTEN__)
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      created = vulkan_create(graphics);
      break;
#endif
#if defined(MLN_TG_HAS_EGL)
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
      created = egl_create(graphics);
      break;
#endif
#if defined(MLN_TG_HAS_WGL)
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      created = wgl_create(graphics);
      break;
#endif
    default:
      set_error("this platform has no %s", backend_name(backend));
      break;
  }
  if (!created) {
    // Keeps the reason across the teardown, which may record one of its own.
    char reason[sizeof(last_error)];
    memcpy(reason, last_error, sizeof(reason));
    mln_test_graphics_destroy(graphics);
    set_error("%s: %s", backend_name(backend), reason);
    return NULL;
  }
  return graphics;
}

void mln_test_graphics_destroy(mln_test_graphics* graphics) {
  if (graphics == NULL) return;
  switch (graphics->context.backend) {
#if defined(MLN_TG_HAS_METAL)
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      metal_destroy(graphics);
      break;
#endif
#if !defined(__EMSCRIPTEN__)
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      vulkan_destroy(graphics);
      break;
#endif
#if defined(MLN_TG_HAS_EGL)
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
      egl_destroy(graphics);
      break;
#endif
#if defined(MLN_TG_HAS_WGL)
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      wgl_destroy(graphics);
      break;
#endif
    default:
      break;
  }
  free(graphics);
}

bool mln_test_graphics_get_context(
  const mln_test_graphics* graphics, mln_test_graphics_context* out_context
) {
  if (graphics == NULL || out_context == NULL) {
    set_error("null argument");
    return false;
  }
  *out_context = graphics->context;
  return true;
}

bool mln_test_graphics_make_current(mln_test_graphics* graphics) {
  if (graphics == NULL) {
    set_error("null argument");
    return false;
  }
#if defined(MLN_TG_HAS_EGL)
  if (graphics->context.backend == MLN_TEST_GRAPHICS_BACKEND_EGL) {
    return egl_make_current(graphics);
  }
#endif
#if defined(MLN_TG_HAS_WGL)
  if (graphics->context.backend == MLN_TEST_GRAPHICS_BACKEND_WGL) {
    return wgl_make_current(graphics);
  }
#endif
  set_error(
    "%s has no current context", backend_name(graphics->context.backend)
  );
  return false;
}

mln_test_graphics_texture* mln_test_graphics_texture_create(
  mln_test_graphics* graphics, uint32_t width, uint32_t height
) {
  if (graphics == NULL || width == 0 || height == 0) {
    set_error("a texture needs a graphics object and a nonzero size");
    return NULL;
  }
  mln_test_graphics_texture* texture = calloc(1, sizeof(*texture));
  if (texture == NULL) {
    set_error("out of memory");
    return NULL;
  }
  texture->graphics = graphics;
  texture->info.width = width;
  texture->info.height = height;
  bool created = false;
  switch (graphics->context.backend) {
#if defined(MLN_TG_HAS_METAL)
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      created = metal_texture_create(graphics, texture);
      break;
#endif
#if !defined(__EMSCRIPTEN__)
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      created = vulkan_texture_create(graphics, texture);
      break;
#endif
#if defined(MLN_TG_HAS_GL)
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      created = gl_texture_create(graphics, texture);
      break;
#endif
    default:
      break;
  }
  if (!created) {
    char reason[sizeof(last_error)];
    memcpy(reason, last_error, sizeof(reason));
    mln_test_graphics_texture_destroy(texture);
    set_error("%s", reason);
    return NULL;
  }
  return texture;
}

void mln_test_graphics_texture_destroy(mln_test_graphics_texture* texture) {
  if (texture == NULL) return;
  switch (texture->graphics->context.backend) {
#if defined(MLN_TG_HAS_METAL)
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      metal_texture_destroy(texture);
      break;
#endif
#if !defined(__EMSCRIPTEN__)
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      vulkan_texture_destroy(texture);
      break;
#endif
#if defined(MLN_TG_HAS_GL)
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      gl_texture_destroy(texture);
      break;
#endif
    default:
      break;
  }
  free(texture);
}

bool mln_test_graphics_texture_get_info(
  const mln_test_graphics_texture* texture,
  mln_test_graphics_texture_info* out_info
) {
  if (texture == NULL || out_info == NULL) {
    set_error("null argument");
    return false;
  }
  *out_info = texture->info;
  return true;
}

bool mln_test_graphics_texture_read_rgba8(
  mln_test_graphics_texture* texture, uint8_t* pixels, size_t size
) {
  if (texture == NULL || pixels == NULL) {
    set_error("null argument");
    return false;
  }
  if (size / 4 / texture->info.width < texture->info.height) {
    set_error(
      "a %ux%u readback needs %zu bytes", texture->info.width,
      texture->info.height,
      (size_t)texture->info.width * texture->info.height * 4
    );
    return false;
  }
  switch (texture->graphics->context.backend) {
#if defined(MLN_TG_HAS_METAL)
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      return metal_texture_read(texture, pixels);
#endif
#if !defined(__EMSCRIPTEN__)
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      return vulkan_texture_read(texture, pixels);
#endif
#if defined(MLN_TG_HAS_GL)
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      return gl_texture_read(texture, pixels);
#endif
    default:
      set_error("unknown backend");
      return false;
  }
}

mln_test_graphics_surface* mln_test_graphics_surface_create(
  mln_test_graphics* graphics, uint32_t width, uint32_t height
) {
  if (graphics == NULL || width == 0 || height == 0) {
    set_error("a surface needs a graphics object and a nonzero size");
    return NULL;
  }
  mln_test_graphics_surface* surface = calloc(1, sizeof(*surface));
  if (surface == NULL) {
    set_error("out of memory");
    return NULL;
  }
  surface->graphics = graphics;
  surface->info.width = width;
  surface->info.height = height;
  bool created = false;
  switch (graphics->context.backend) {
#if defined(MLN_TG_HAS_METAL)
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      created = metal_surface_create(graphics, surface);
      break;
#endif
#if !defined(__EMSCRIPTEN__)
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      created = vulkan_surface_create(graphics, surface);
      break;
#endif
#if defined(MLN_TG_HAS_EGL)
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
      created = egl_surface_create(graphics, surface);
      break;
#endif
#if defined(MLN_TG_HAS_WGL)
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      created = wgl_surface_create(graphics, surface);
      break;
#endif
    default:
      break;
  }
  if (!created) {
    char reason[sizeof(last_error)];
    memcpy(reason, last_error, sizeof(reason));
    mln_test_graphics_surface_destroy(surface);
    set_error("%s", reason);
    return NULL;
  }
  return surface;
}

void mln_test_graphics_surface_destroy(mln_test_graphics_surface* surface) {
  if (surface == NULL) return;
  switch (surface->graphics->context.backend) {
#if defined(MLN_TG_HAS_METAL)
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      metal_surface_destroy(surface);
      break;
#endif
#if !defined(__EMSCRIPTEN__)
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      vulkan_surface_destroy(surface);
#if defined(MLN_TG_HAS_WGL)
      if (surface->window != NULL) DestroyWindow(surface->window);
#endif
      break;
#endif
#if defined(MLN_TG_HAS_EGL)
    case MLN_TEST_GRAPHICS_BACKEND_EGL:
      egl_surface_destroy(surface);
      break;
#endif
#if defined(MLN_TG_HAS_WGL)
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      wgl_surface_destroy(surface);
      break;
#endif
    default:
      break;
  }
  free(surface);
}

bool mln_test_graphics_surface_get_info(
  const mln_test_graphics_surface* surface,
  mln_test_graphics_surface_info* out_info
) {
  if (surface == NULL || out_info == NULL) {
    set_error("null argument");
    return false;
  }
  *out_info = surface->info;
  return true;
}
