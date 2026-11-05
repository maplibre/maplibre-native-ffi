// The host's GPU: a device or context from tests/graphics, the textures of
// each session's ring, and a description of the device for the run's context.

#include <cstdio>
#include <cstring>
#include <string>

#include "bench.hpp"

#if defined(__EMSCRIPTEN__)

#include <cstdlib>

#include <emscripten/em_js.h>

EM_JS_DEPS(mln_bench_gpu, "$stringToNewUTF8");

// The browser's default adapter, which an implementation gets when it asks for
// none in particular. The formatter would mangle the JavaScript.
// clang-format off
EM_ASYNC_JS(char*, bench_browser_adapter, (), {
  try {
    const adapter = await navigator.gpu?.requestAdapter();
    if (!adapter) return 0;
    const info = adapter.info ?? {};
    const software =
      info.isFallbackAdapter === true ||
      /swiftshader|llvmpipe|lavapipe|software/i.test(
        `${info.architecture} ${info.description} ${info.device}`
      );
    const name =
      [info.vendor, info.architecture, info.device, info.description]
        .filter((part) => part)
        .join(" ") || "unnamed adapter";
    return stringToNewUTF8(`${software ? "1" : "0"}${name}`);
  } catch (error) {
    return 0;
  }
});
// clang-format on

namespace bench {

void gpu_describe(Environment& environment) {
  environment.render_target = "offscreen";
  environment.gpu_device = "the browser's, unknown";
  environment.gpu_type = "unknown";
  if (char* adapter = bench_browser_adapter()) {
    environment.software_gpu = adapter[0] == '1';
    environment.gpu_device =
      std::string{"the browser's default adapter: "} + (adapter + 1);
    environment.gpu_type = environment.software_gpu ? "cpu" : "unknown";
    std::free(adapter);
  }
}

auto gpu_has_ring() -> bool { return false; }
auto gpu_driver() -> mln_render_driver_kind {
  return MLN_RENDER_DRIVER_CORE_WORKER;
}
auto gpu_producer_sync_kinds() -> mln_gpu_sync_kind_flags { return 0; }
auto gpu_frame_slot(mln_acquired_frame) -> int { return -1; }
auto gpu_wait(const mln_gpu_sync& sync) -> bool {
  return sync.kind == MLN_GPU_SYNC_CPU_COMPLETE;
}
auto gpu_read_texture(void*, Image&) -> bool { return false; }

void gpu_create_ring(void**, uint32_t* texture_count) { *texture_count = 0; }

auto gpu_attach_ring(
  mln_map, const mln_render_session_attach_options&, void* const*,
  mln_render_session*, const mln_completion*, mln_diagnostic*
) -> mln_status {
  return MLN_STATUS_UNSUPPORTED;
}

void gpu_destroy_ring(void**, uint32_t) {}

}  // namespace bench

#else

#include "mln_test_graphics.h"

#if defined(BENCH_BACKEND_VULKAN)
#include <vulkan/vulkan.h>
#endif
#if defined(BENCH_BACKEND_METAL)
#include <objc/message.h>
#include <objc/runtime.h>
#endif

namespace bench {

namespace {

#if defined(BENCH_BACKEND_METAL)
constexpr uint32_t kHostBackend = MLN_TEST_GRAPHICS_BACKEND_METAL;
#elif defined(BENCH_BACKEND_VULKAN)
constexpr uint32_t kHostBackend = MLN_TEST_GRAPHICS_BACKEND_VULKAN;
#elif defined(BENCH_OPENGL_WGL)
constexpr uint32_t kHostBackend = MLN_TEST_GRAPHICS_BACKEND_WGL;
#define BENCH_HOST_OPENGL 1
#else
constexpr uint32_t kHostBackend = MLN_TEST_GRAPHICS_BACKEND_EGL;
#define BENCH_HOST_OPENGL 1
#endif

struct Host {
  mln_test_graphics* graphics = nullptr;
  mln_test_graphics_context context{};
};

// Created once and kept until the process exits. An OpenGL context is current
// on the benchmark thread, which services every caller-driven session.
auto host() -> Host& {
  static Host value = [] {
    Host created;
    created.graphics = mln_test_graphics_create(kHostBackend);
    if (
      created.graphics == nullptr ||
      !mln_test_graphics_get_context(created.graphics, &created.context)
    ) {
      fail("tests/graphics has no device: %s", mln_test_graphics_last_error());
    }
#if defined(BENCH_HOST_OPENGL)
    if (!mln_test_graphics_make_current(created.graphics)) {
      fail(
        "the OpenGL context is not current: %s", mln_test_graphics_last_error()
      );
    }
#endif
    return created;
  }();
  return value;
}

#if defined(BENCH_BACKEND_VULKAN)
auto vulkan_type_name(VkPhysicalDeviceType type) -> const char* {
  switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      return "discrete";
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      return "integrated";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      return "virtual";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      return "cpu";
    default:
      return "other";
  }
}

void describe_vulkan(
  const mln_test_graphics_context& context, Environment& out
) {
  const auto get_instance_proc_addr =
    reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      context.vulkan_get_instance_proc_addr
    );
  const auto instance = static_cast<VkInstance>(context.vulkan_instance);
  const auto get_properties =
    reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
      get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties")
    );
  const auto enumerate = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
    get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices")
  );
  const auto chosen =
    static_cast<VkPhysicalDevice>(context.vulkan_physical_device);
  VkPhysicalDeviceProperties properties{};
  get_properties(chosen, &properties);
  out.gpu_device = properties.deviceName;
  out.gpu_type = vulkan_type_name(properties.deviceType);
  out.software_gpu = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
  const auto get_properties2 =
    reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
      get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties2")
    );
  if (
    get_properties2 != nullptr && properties.apiVersion >= VK_API_VERSION_1_2
  ) {
    VkPhysicalDeviceDriverProperties driver{};
    driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    VkPhysicalDeviceProperties2 extended{};
    extended.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    extended.pNext = &driver;
    get_properties2(chosen, &extended);
    out.gpu_driver = std::string{driver.driverName} + " " + driver.driverInfo;
  }
  // An offscreen device names its GPU by its index in the same list.
  uint32_t count = 0;
  enumerate(instance, &count, nullptr);
  VkPhysicalDevice devices[16] = {};
  count = count > 16 ? 16 : count;
  enumerate(instance, &count, devices);
  for (uint32_t index = 0; index < count; ++index) {
    if (devices[index] == chosen) {
      out.has_offscreen_adapter = true;
      out.offscreen_adapter_index = index;
    }
  }
}
#endif

#if defined(BENCH_BACKEND_METAL)
void describe_metal(
  const mln_test_graphics_context& context, Environment& out
) {
  using Send = id (*)(id, SEL);
  using SendText = const char* (*)(id, SEL);
  id name = reinterpret_cast<Send>(objc_msgSend)(
    static_cast<id>(context.metal_device), sel_registerName("name")
  );
  if (name != nil) {
    out.gpu_device = reinterpret_cast<SendText>(objc_msgSend)(
      name, sel_registerName("UTF8String")
    );
  }
  out.gpu_type = "metal";
}
#endif

#if defined(BENCH_HOST_OPENGL)
void describe_opengl(
  const mln_test_graphics_context& context, Environment& out
) {
  using GetProcAddress = void* (*)(const char*);
  using GetString = const unsigned char* (*)(unsigned int);
  constexpr unsigned int kRenderer = 0x1F01;
  if (context.get_proc_address == nullptr) return;
  const auto get_string = reinterpret_cast<GetString>(
    reinterpret_cast<GetProcAddress>(context.get_proc_address)("glGetString")
  );
  if (get_string == nullptr) return;
  if (const auto* renderer = get_string(kRenderer)) {
    out.gpu_device = reinterpret_cast<const char*>(renderer);
    out.software_gpu = out.gpu_device.find("llvmpipe") != std::string::npos ||
                       out.gpu_device.find("SwiftShader") != std::string::npos;
    out.gpu_type = out.software_gpu ? "cpu" : "gpu";
  }
}
#endif

}  // namespace

void gpu_describe(Environment& environment) {
  environment.render_target = "borrowed texture ring";
  environment.gpu_device = "unknown";
  environment.gpu_type = "unknown";
  const auto& context = host().context;
#if defined(BENCH_BACKEND_VULKAN)
  describe_vulkan(context, environment);
#elif defined(BENCH_BACKEND_METAL)
  describe_metal(context, environment);
#else
  describe_opengl(context, environment);
#endif
}

auto gpu_has_ring() -> bool { return true; }

auto gpu_driver() -> mln_render_driver_kind {
#if defined(BENCH_HOST_OPENGL)
  return MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
#else
  return MLN_RENDER_DRIVER_CORE_WORKER;
#endif
}

// A sync that the host waits on before it reads a frame, as a compositor
// would, so that each frame's time includes its GPU work.
auto gpu_producer_sync_kinds() -> mln_gpu_sync_kind_flags {
#if defined(BENCH_BACKEND_VULKAN)
  return MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE |
         MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE;
#elif defined(BENCH_BACKEND_METAL)
  return MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT |
         MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE;
#else
  return MLN_GPU_SYNC_KIND_FLAG_OPEN_GL_FENCE |
         MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE;
#endif
}

auto gpu_frame_slot(mln_acquired_frame frame) -> int {
#if defined(BENCH_BACKEND_VULKAN)
  mln_vulkan_texture_frame texture{};
  texture.size = sizeof(texture);
  const auto status =
    mln_acquired_frame_get_vulkan_texture(frame, &texture, nullptr);
#elif defined(BENCH_BACKEND_METAL)
  mln_metal_texture_frame texture{};
  texture.size = sizeof(texture);
  const auto status =
    mln_acquired_frame_get_metal_texture(frame, &texture, nullptr);
#else
  mln_open_gl_texture_frame texture{};
  texture.size = sizeof(texture);
  const auto status =
    mln_acquired_frame_get_open_gl_texture(frame, &texture, nullptr);
#endif
  return status == MLN_STATUS_OK ? static_cast<int>(texture.slot) : -1;
}

auto gpu_wait(const mln_gpu_sync& sync) -> bool {
  // An emulator that draws in software can take seconds over a frame, so the
  // wait allows as long as any other wait in a case.
  constexpr auto kTimeoutNs = static_cast<uint64_t>(
    std::chrono::duration_cast<std::chrono::nanoseconds>(kWaitLimit).count()
  );
  switch (sync.kind) {
    case MLN_GPU_SYNC_CPU_COMPLETE:
      return true;
#if defined(BENCH_BACKEND_VULKAN)
    case MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE:
      if (
        mln_test_graphics_vulkan_timeline_wait(
          host().graphics, sync.handle, sync.value, kTimeoutNs
        ) != 1
      ) {
        fail("the frame's timeline semaphore never signaled");
      }
      return true;
#endif
#if defined(BENCH_BACKEND_METAL)
    case MLN_GPU_SYNC_METAL_SHARED_EVENT: {
      using Wait = bool (*)(id, SEL, uint64_t, uint64_t);
      const auto signaled = reinterpret_cast<Wait>(objc_msgSend)(
        reinterpret_cast<id>(sync.handle),
        sel_registerName("waitUntilSignaledValue:timeoutMS:"), sync.value,
        kTimeoutNs / 1000000ULL
      );
      if (!signaled) fail("the frame's shared event never signaled");
      return true;
    }
#endif
#if defined(BENCH_HOST_OPENGL)
    case MLN_GPU_SYNC_OPEN_GL_FENCE: {
      using GetProcAddress = void* (*)(const char*);
      using ClientWaitSync = unsigned int (*)(void*, unsigned int, uint64_t);
      constexpr unsigned int kFlushCommands = 0x00000001;
      constexpr unsigned int kAlreadySignaled = 0x911A;
      constexpr unsigned int kConditionSatisfied = 0x911C;
      static const auto client_wait = reinterpret_cast<ClientWaitSync>(
        reinterpret_cast<GetProcAddress>(host().context.get_proc_address)(
          "glClientWaitSync"
        )
      );
      if (client_wait == nullptr) return false;
      const auto result = client_wait(
        reinterpret_cast<void*>(sync.handle), kFlushCommands, kTimeoutNs
      );
      if (result != kAlreadySignaled && result != kConditionSatisfied) {
        fail("the frame's fence never signaled");
      }
      return true;
    }
#endif
    default:
      return false;
  }
}

auto gpu_read_texture(void* texture, Image& image) -> bool {
  auto* graphics_texture = static_cast<mln_test_graphics_texture*>(texture);
  mln_test_graphics_texture_info info{};
  if (!mln_test_graphics_texture_get_info(graphics_texture, &info))
    return false;
  image.width = info.width;
  image.height = info.height;
  image.rgba.resize(static_cast<size_t>(info.width) * info.height * 4);
  return mln_test_graphics_texture_read_rgba8(
    graphics_texture, image.rgba.data(), image.rgba.size()
  );
}

constexpr uint32_t kDepth = 2;

void gpu_create_ring(void** textures, uint32_t* texture_count) {
  auto& graphics = host();
  for (uint32_t index = 0; index < kDepth; ++index) {
    auto* texture =
      mln_test_graphics_texture_create(graphics.graphics, kWidth, kHeight);
    if (texture == nullptr) {
      gpu_destroy_ring(textures, index);
      fail(
        "tests/graphics cannot create a texture: %s",
        mln_test_graphics_last_error()
      );
    }
    textures[index] = texture;
  }
  *texture_count = kDepth;
}

namespace {

// What every backend's borrowed texture descriptor shares.
template <typename Descriptor, typename Entry>
void describe_ring(
  Descriptor& descriptor, Entry* entries,
  const mln_test_graphics_texture_info& info
) {
  descriptor.size = sizeof(descriptor);
  descriptor.logical_size = {info.width, info.height};
  descriptor.physical_width = info.width;
  descriptor.physical_height = info.height;
  descriptor.textures = entries;
  descriptor.texture_count = kDepth;
}

}  // namespace

auto gpu_attach_ring(
  mln_map map, const mln_render_session_attach_options& options,
  void* const* textures, mln_render_session* out_session,
  const mln_completion* completion, mln_diagnostic* diagnostic
) -> mln_status {
  mln_test_graphics_texture_info info[kDepth] = {};
  for (uint32_t index = 0; index < kDepth; ++index) {
    mln_test_graphics_texture_get_info(
      static_cast<mln_test_graphics_texture*>(textures[index]), &info[index]
    );
  }
#if defined(BENCH_BACKEND_VULKAN)
  mln_vulkan_borrowed_texture entries[kDepth] = {};
  for (uint32_t index = 0; index < kDepth; ++index) {
    entries[index].image = info[index].vulkan_image;
    entries[index].image_view = info[index].vulkan_image_view;
  }
  const auto& context = host().context;
  mln_vulkan_borrowed_texture_descriptor descriptor{};
  describe_ring(descriptor, entries, info[0]);
  descriptor.context.instance = context.vulkan_instance;
  descriptor.context.physical_device = context.vulkan_physical_device;
  descriptor.context.device = context.vulkan_device;
  descriptor.context.graphics_queue = context.vulkan_queue;
  descriptor.context.graphics_queue_family_index =
    context.vulkan_queue_family_index;
  descriptor.context.get_instance_proc_addr =
    context.vulkan_get_instance_proc_addr;
  descriptor.context.get_device_proc_addr = context.vulkan_get_device_proc_addr;
  descriptor.format = info[0].format;
  descriptor.final_layout = info[0].vulkan_final_layout;
  return mln_map_attach_vulkan_borrowed_texture(
    map, &descriptor, &options, out_session, completion, diagnostic
  );
#elif defined(BENCH_BACKEND_METAL)
  mln_metal_borrowed_texture entries[kDepth] = {};
  for (uint32_t index = 0; index < kDepth; ++index) {
    entries[index].texture = info[index].metal_texture;
  }
  mln_metal_borrowed_texture_descriptor descriptor{};
  describe_ring(descriptor, entries, info[0]);
  return mln_map_attach_metal_borrowed_texture(
    map, &descriptor, &options, out_session, completion, diagnostic
  );
#else
  mln_open_gl_borrowed_texture entries[kDepth] = {};
  for (uint32_t index = 0; index < kDepth; ++index) {
    entries[index].texture = info[index].opengl_texture;
  }
  const auto& context = host().context;
  mln_open_gl_borrowed_texture_descriptor descriptor{};
  describe_ring(descriptor, entries, info[0]);
#if defined(BENCH_OPENGL_WGL)
  descriptor.context.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WGL;
  descriptor.context.data.wgl.device_context = context.wgl_device_context;
  descriptor.context.data.wgl.share_context = context.wgl_context;
  descriptor.context.data.wgl.get_proc_address = context.get_proc_address;
#else
  descriptor.context.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL;
  descriptor.context.data.egl.display = context.egl_display;
  descriptor.context.data.egl.config = context.egl_config;
  descriptor.context.data.egl.share_context = context.egl_context;
#endif
  descriptor.target = info[0].opengl_target;
  return mln_map_attach_open_gl_borrowed_texture(
    map, &descriptor, &options, out_session, completion, diagnostic
  );
#endif
}

void gpu_destroy_ring(void** textures, uint32_t texture_count) {
  for (uint32_t index = 0; index < texture_count; ++index) {
    mln_test_graphics_texture_destroy(
      static_cast<mln_test_graphics_texture*>(textures[index])
    );
    textures[index] = nullptr;
  }
}

}  // namespace bench

#endif
