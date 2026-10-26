// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan backend context fields shared by Vulkan render targets.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct VulkanContextDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanContextDescriptor(
        NativePointer Instance,
        NativePointer PhysicalDevice,
        NativePointer Device,
        NativePointer GraphicsQueue,
        uint GraphicsQueueFamilyIndex,
        NativePointer GetInstanceProcAddr,
        NativePointer GetDeviceProcAddr
    )
    {
        this.Instance = Instance;
        this.PhysicalDevice = PhysicalDevice;
        this.Device = Device;
        this.GraphicsQueue = GraphicsQueue;
        this.GraphicsQueueFamilyIndex = GraphicsQueueFamilyIndex;
        this.GetInstanceProcAddr = GetInstanceProcAddr;
        this.GetDeviceProcAddr = GetDeviceProcAddr;
    }

    /// <summary>
    /// Borrowed VkInstance. Required.
    /// </summary>
    public required NativePointer Instance { get; init; }

    /// <summary>
    /// Borrowed VkPhysicalDevice. Required.
    /// </summary>
    public required NativePointer PhysicalDevice { get; init; }

    /// <summary>
    /// Borrowed VkDevice. Required.
    /// </summary>
    public required NativePointer Device { get; init; }

    /// <summary>
    /// Borrowed graphics VkQueue. Required. The session's driver submits to it
    /// from its own thread, so a host that uses the same queue passes
    /// <c>RenderSessionAttachOptions.QueueLock</c> at attach.
    /// </summary>
    public required NativePointer GraphicsQueue { get; init; }

    /// <summary>
    /// Queue family index for graphics_queue. Must support graphics commands.
    /// </summary>
    public uint GraphicsQueueFamilyIndex { get; init; }

    /// <summary>
    /// PFN_vkGetInstanceProcAddr for the loader that created the instance.
    /// </summary>
    public NativePointer GetInstanceProcAddr { get; init; }

    /// <summary>
    /// PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device.
    /// </summary>
    public NativePointer GetDeviceProcAddr { get; init; }
}
