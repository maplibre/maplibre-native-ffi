// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The frame's Vulkan command buffer, outside the map's render pass.
/// </summary>
/// <remarks>
/// <para>Record work such as copies, compute dispatches, and render passes of
/// your own, with the barriers that it needs. The map's render pass loads its
/// attachments after a barrier of its own, but it doesn't wait for writes to
/// your images: give those the barriers that your later reads need. Don't end
/// or submit <c>command_buffer</c>.</para>
/// <para>See <c>mln_vulkan_custom_layer_encoder</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanCustomLayerEncoder
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanCustomLayerEncoder(VulkanContextDescriptor Context, NativePointer CommandBuffer)
    {
        this.Context = Context;
        this.CommandBuffer = CommandBuffer;
    }

    /// <summary>
    /// The device that the session renders on. Make the objects that your
    /// commands use on <c>context.device</c>. Don't submit to
    /// <c>context.graphics_queue</c> from a callback.
    /// </summary>
    public required VulkanContextDescriptor Context { get; init; }

    /// <summary>
    /// The frame's <c>VkCommandBuffer</c>, recording outside a render pass.
    /// </summary>
    public NativePointer CommandBuffer { get; init; }
}
