// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Vulkan objects of a device, as <c>GpuDeviceHandle.GetVulkanContext()</c>
/// reports them.
/// </summary>
/// <remarks>
/// See <c>mln_gpu_device_vulkan_context_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gpu__device_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct GpuDeviceVulkanContextResult
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public GpuDeviceVulkanContextResult(VulkanContextDescriptor Context)
    {
        this.Context = Context;
    }

    public required VulkanContextDescriptor Context { get; init; }
}
