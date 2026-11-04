// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Vulkan device's context, as <c>VulkanDeviceHandle.GetContext()</c> reports
/// it.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_device_context_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/vulkan_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct VulkanDeviceContextResult
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanDeviceContextResult(VulkanContextDescriptor Context)
    {
        this.Context = Context;
    }

    public required VulkanContextDescriptor Context { get; init; }
}
