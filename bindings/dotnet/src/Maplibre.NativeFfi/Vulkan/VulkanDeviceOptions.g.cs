// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for <c>VulkanDeviceHandle.Create()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_device_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html">C API reference</see>.
/// </remarks>
public sealed record VulkanDeviceOptions
{
    /// <summary>
    /// Whether the device can present to a window's surface. It enables
    /// VK_KHR_surface and the platform's surface extensions on the instance,
    /// and VK_KHR_swapchain on the device. Defaults to false.
    /// </summary>
    public bool? Presentation { get; init; }

    /// <summary>
    /// The 16-byte <c>deviceUUID</c> of the GPU to use, as
    /// <c>VkPhysicalDeviceIDProperties</c> reports it. Name a GPU to share
    /// textures with another graphics API on it.
    /// </summary>
    public ImmutableArray<byte>? DeviceUuid
    {
        get => DeviceUuidStorage?.AsImmutable();
        init => DeviceUuidStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<byte>? DeviceUuidStorage { get; init; }

    /// <summary>
    /// The 8-byte <c>deviceLUID</c> of the GPU to use, as
    /// <c>VkPhysicalDeviceIDProperties</c> reports it. On Windows, this is the
    /// GPU's DXGI adapter LUID.
    /// </summary>
    public ImmutableArray<byte>? DeviceLuid
    {
        get => DeviceLuidStorage?.AsImmutable();
        init => DeviceLuidStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<byte>? DeviceLuidStorage { get; init; }
}
