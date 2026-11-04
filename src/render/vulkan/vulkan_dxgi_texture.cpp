#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// The D3D and Vulkan Win32 headers name types that windows.h declares.
#include <d3d11.h>
#include <dxgi1_4.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c/base.h"
#include "render/vulkan/vulkan_dxgi_texture.hpp"

namespace mln::core {

class DxgiTextureDevice {
 public:
  explicit DxgiTextureDevice(ID3D11Device* device) : device_(device) {}
  DxgiTextureDevice(const DxgiTextureDevice&) = delete;
  auto operator=(const DxgiTextureDevice&) -> DxgiTextureDevice& = delete;
  DxgiTextureDevice(DxgiTextureDevice&&) = delete;
  auto operator=(DxgiTextureDevice&&) -> DxgiTextureDevice& = delete;
  ~DxgiTextureDevice() { device_->Release(); }

  [[nodiscard]] auto get() const -> ID3D11Device* { return device_; }

 private:
  ID3D11Device* device_;
};

namespace {

// Releases a COM object when it leaves scope.
template <typename Object>
class ComRef {
 public:
  ComRef() = default;
  ComRef(const ComRef&) = delete;
  auto operator=(const ComRef&) -> ComRef& = delete;
  ComRef(ComRef&&) = delete;
  auto operator=(ComRef&&) -> ComRef& = delete;
  ~ComRef() {
    if (object_ != nullptr) object_->Release();
  }

  [[nodiscard]] auto get() const -> Object* { return object_; }
  auto out() -> Object** { return &object_; }
  auto release() -> Object* { return std::exchange(object_, nullptr); }
  auto operator->() const -> Object* { return object_; }

 private:
  Object* object_ = nullptr;
};

// A shared texture of `width` by `height` and its NT handle, or false.
auto create_shared_texture(
  ID3D11Device* device, uint32_t width, uint32_t height,
  ID3D11Texture2D*& out_texture, HANDLE& out_handle
) -> bool {
  const auto description = D3D11_TEXTURE2D_DESC{
    .Width = width,
    .Height = height,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format = DXGI_FORMAT_B8G8R8A8_UNORM,
    .SampleDesc = {.Count = 1, .Quality = 0},
    .Usage = D3D11_USAGE_DEFAULT,
    .BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
    .CPUAccessFlags = 0,
    .MiscFlags =
      D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
  };
  auto texture = ComRef<ID3D11Texture2D>{};
  auto resource = ComRef<IDXGIResource1>{};
  auto handle = HANDLE{nullptr};
  if (
    FAILED(device->CreateTexture2D(&description, nullptr, texture.out())) ||
    FAILED(texture->QueryInterface(IID_PPV_ARGS(resource.out()))) ||
    FAILED(resource->CreateSharedHandle(
      nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr,
      &handle
    ))
  )
    return false;
  out_texture = texture.release();
  out_handle = handle;
  return true;
}

}  // namespace

auto create_dxgi_texture_device(
  const std::array<uint8_t, 8>& luid,
  std::shared_ptr<const DxgiTextureDevice>& out_device
) -> mln_status {
  // Load each module once and keep it for the process, like a linked import.
  static auto* const dxgi = LoadLibraryA("dxgi.dll");
  static auto* const d3d11 = LoadLibraryA("d3d11.dll");
  if (dxgi == nullptr || d3d11 == nullptr) {
    set_thread_error("D3D11 is not available");
    return MLN_STATUS_UNSUPPORTED;
  }
  using CreateFactory = HRESULT(WINAPI*)(REFIID, void**);
  // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto create_factory =
    reinterpret_cast<CreateFactory>(GetProcAddress(dxgi, "CreateDXGIFactory1"));
  const auto create_device = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(
    GetProcAddress(d3d11, "D3D11CreateDevice")
  );
  // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)
  if (create_factory == nullptr || create_device == nullptr) {
    set_thread_error("D3D11 is not available");
    return MLN_STATUS_UNSUPPORTED;
  }
  auto adapter_luid = LUID{};
  std::memcpy(&adapter_luid, luid.data(), sizeof(adapter_luid));
  auto factory = ComRef<IDXGIFactory4>{};
  auto adapter = ComRef<IDXGIAdapter>{};
  if (
    FAILED(create_factory(IID_PPV_ARGS(factory.out()))) ||
    FAILED(
      factory->EnumAdapterByLuid(adapter_luid, IID_PPV_ARGS(adapter.out()))
    )
  ) {
    set_thread_error("D3D11 has no adapter with the Vulkan device's LUID");
    return MLN_STATUS_UNSUPPORTED;
  }
  auto device = ComRef<ID3D11Device>{};
  if (
    FAILED(create_device(
      adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0,
      D3D11_SDK_VERSION, device.out(), nullptr, nullptr
    ))
  ) {
    set_thread_error("D3D11 can't create a device on the Vulkan GPU's adapter");
    return MLN_STATUS_NATIVE_ERROR;
  }
  // A driver may create shared textures but not NT handles to them, as Wine's
  // does.
  auto* texture = static_cast<ID3D11Texture2D*>(nullptr);
  auto handle = HANDLE{nullptr};
  if (!create_shared_texture(device.get(), 1, 1, texture, handle)) {
    set_thread_error("D3D11 can't share textures through NT handles");
    return MLN_STATUS_UNSUPPORTED;
  }
  CloseHandle(handle);
  texture->Release();
  out_device = std::make_shared<const DxgiTextureDevice>(device.release());
  return MLN_STATUS_OK;
}

DxgiSharedTexture::DxgiSharedTexture(
  std::shared_ptr<const DxgiTextureDevice> device, uint32_t width,
  uint32_t height
)
    : device_(std::move(device)) {
  auto query = ComRef<ID3D11Query>{};
  const auto done =
    D3D11_QUERY_DESC{.Query = D3D11_QUERY_EVENT, .MiscFlags = 0};
  if (FAILED(device_->get()->CreateQuery(&done, query.out()))) {
    throw std::runtime_error("D3D11 can't create a query");
  }
  auto* texture = static_cast<ID3D11Texture2D*>(nullptr);
  auto handle = HANDLE{nullptr};
  if (!create_shared_texture(device_->get(), width, height, texture, handle)) {
    throw std::runtime_error("D3D11 can't create a shared texture");
  }
  texture_ = texture;
  handle_ = handle;
  // A driver may initialize the texture with work on the immediate context,
  // as DXVK does, which must not land after Vulkan renders into it.
  auto context = ComRef<ID3D11DeviceContext>{};
  device_->get()->GetImmediateContext(context.out());
  context->End(query.get());
  context->Flush();
  auto finished = BOOL{FALSE};
  while (context->GetData(query.get(), &finished, sizeof(finished), 0) ==
         S_FALSE) {
    SwitchToThread();
  }
}

DxgiSharedTexture::~DxgiSharedTexture() {
  if (memory_ != VK_NULL_HANDLE) free_memory_(memory_device_, memory_, nullptr);
  // Vulkan's import leaves the NT handle to its caller.
  if (handle_ != nullptr) CloseHandle(handle_);
  if (texture_ != nullptr) static_cast<ID3D11Texture2D*>(texture_)->Release();
}

void DxgiSharedTexture::bind(
  VkDevice device, PFN_vkGetDeviceProcAddr get_proc,
  const VkPhysicalDeviceMemoryProperties& memory, VkImage image
) {
  constexpr auto handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
  const auto proc = [&](auto& out, const char* name) {
    // Vulkan hands out its functions as untyped procedure pointers.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    out = reinterpret_cast<std::remove_reference_t<decltype(out)>>(
      get_proc(device, name)
    );
    return out != nullptr;
  };
  auto get_handle_properties = PFN_vkGetMemoryWin32HandlePropertiesKHR{};
  auto get_requirements = PFN_vkGetImageMemoryRequirements{};
  auto allocate = PFN_vkAllocateMemory{};
  auto bind_memory = PFN_vkBindImageMemory{};
  auto free_memory = PFN_vkFreeMemory{};
  if (
    !proc(get_handle_properties, "vkGetMemoryWin32HandlePropertiesKHR") ||
    !proc(get_requirements, "vkGetImageMemoryRequirements") ||
    !proc(allocate, "vkAllocateMemory") ||
    !proc(bind_memory, "vkBindImageMemory") ||
    !proc(free_memory, "vkFreeMemory")
  ) {
    throw std::runtime_error("Vulkan can't import D3D11 textures");
  }
  auto properties = VkMemoryWin32HandlePropertiesKHR{
    .sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR,
    .pNext = nullptr,
    .memoryTypeBits = 0,
  };
  if (
    get_handle_properties(device, handle_type, handle_, &properties) !=
    VK_SUCCESS
  ) {
    throw std::runtime_error("Vulkan can't import the D3D11 texture");
  }
  auto requirements = VkMemoryRequirements{};
  get_requirements(device, image, &requirements);
  const auto types = requirements.memoryTypeBits & properties.memoryTypeBits;
  if (types == 0) {
    throw std::runtime_error("no Vulkan memory type imports the D3D11 texture");
  }
  // The GPU's own memory, where the driver can render.
  auto type = static_cast<uint32_t>(std::countr_zero(types));
  for (auto index = uint32_t{0}; index < memory.memoryTypeCount; ++index) {
    if (
      ((types >> index) & 1U) != 0 && (memory.memoryTypes[index].propertyFlags &
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
    ) {
      type = index;
      break;
    }
  }
  // Vulkan imports a D3D11 texture only into an allocation of its own.
  const auto dedicated = VkMemoryDedicatedAllocateInfo{
    .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
    .pNext = nullptr,
    .image = image,
    .buffer = VK_NULL_HANDLE,
  };
  const auto imported = VkImportMemoryWin32HandleInfoKHR{
    .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR,
    .pNext = &dedicated,
    .handleType = handle_type,
    .handle = handle_,
    .name = nullptr,
  };
  const auto info = VkMemoryAllocateInfo{
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = &imported,
    .allocationSize = requirements.size,
    .memoryTypeIndex = type,
  };
  auto allocated = VkDeviceMemory{VK_NULL_HANDLE};
  if (allocate(device, &info, nullptr, &allocated) != VK_SUCCESS) {
    throw std::runtime_error("Vulkan can't import the D3D11 texture's memory");
  }
  if (bind_memory(device, image, allocated, 0) != VK_SUCCESS) {
    free_memory(device, allocated, nullptr);
    throw std::runtime_error("Vulkan can't bind the D3D11 texture's memory");
  }
  memory_device_ = device;
  free_memory_ = free_memory;
  memory_ = allocated;
}

}  // namespace mln::core
