// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your Vulkan device and queue, for <c>GpuDeviceHandle.WrapVulkan()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_device_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct VulkanDeviceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanDeviceDescriptor(
        VulkanContextDescriptor Context,
        QueueLock QueueLock,
        ImmutableArray<string> EnabledExtensions
    )
    {
        this.Context = Context;
        this.QueueLock = QueueLock;
        this.EnabledExtensions = EnabledExtensions;
    }

    public required VulkanContextDescriptor Context { get; init; }

    /// <summary>
    /// Your lock on <c>context.graphics_queue</c>, when you submit to it
    /// yourself. Give every device that wraps the queue the same lock.
    /// </summary>
    public QueueLock QueueLock { get; init; }

    /// <summary>
    /// The names of the device extensions that you enabled on
    /// <c>context.device</c>, such as "VK_KHR_external_semaphore_fd". Vulkan
    /// offers no way to ask a device which extensions it has, so the device
    /// offers the export kinds and <c>GpuSyncKind.SyncFd</c> only when the
    /// extensions that they need are listed here, and none of them when the
    /// list is empty.
    /// </summary>
    public ImmutableArray<string> EnabledExtensions
    {
        get => EnabledExtensionsStorage.AsImmutable();
        init => EnabledExtensionsStorage = ValueArray.From(value);
    }
    internal ValueArray<string> EnabledExtensionsStorage { get; init; }
}
