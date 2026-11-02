// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Vulkan device and queue that a render session uses.
/// </summary>
/// <remarks>
/// <para>Keep these objects alive until a detach or disposal of the session
/// completes, or an abandon of it returns. If you discarded a frame of a ring
/// that the session creates, or abandon the session while you hold a frame,
/// keep them alive until the process exits instead.</para>
/// <para>See <c>mln_vulkan_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
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
    /// A <c>VkInstance</c>.
    /// </summary>
    public required NativePointer Instance { get; init; }

    /// <summary>
    /// A <c>VkPhysicalDevice</c>.
    /// </summary>
    public required NativePointer PhysicalDevice { get; init; }

    /// <summary>
    /// A <c>VkDevice</c>.
    /// </summary>
    public required NativePointer Device { get; init; }

    /// <summary>
    /// A <c>VkQueue</c> from <c>graphics_queue_family_index</c> that supports
    /// graphics. If you submit to it too, set
    /// <c>RenderSessionAttachOptions.QueueLock</c>.
    /// </summary>
    public required NativePointer GraphicsQueue { get; init; }
    public uint GraphicsQueueFamilyIndex { get; init; }

    /// <summary>
    /// Your <c>vkGetInstanceProcAddr</c>, or null to use the Vulkan loader's.
    /// </summary>
    public NativePointer GetInstanceProcAddr { get; init; }

    /// <summary>
    /// Your <c>vkGetDeviceProcAddr</c>, or null to look it up from the
    /// instance.
    /// </summary>
    public NativePointer GetDeviceProcAddr { get; init; }
}
