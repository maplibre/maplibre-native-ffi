// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The map's Vulkan render pass, at a custom layer's place in the style.
/// </summary>
/// <remarks>
/// <para>Record draws into <c>command_buffer</c>, with pipelines that you make
/// for <c>render_pass</c>, <c>subpass</c>, and <c>sample_count</c>. Bind your
/// own pipeline, descriptor sets, vertex and index buffers, and push constants,
/// and set the dynamic state that your pipelines have, such as the viewport and
/// scissor. After the callback returns, the map binds and sets again what its
/// own draws need. Your pipeline layouts can differ from the map's. Record
/// nothing that a render pass can't hold, and don't end the render pass.</para>
/// <para>See <c>mln_vulkan_custom_layer_pass</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanCustomLayerPass
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanCustomLayerPass(
        VulkanContextDescriptor Context,
        NativePointer CommandBuffer,
        ulong RenderPass,
        uint Subpass,
        uint SampleCount,
        uint ColorFormat,
        uint DepthStencilFormat
    )
    {
        this.Context = Context;
        this.CommandBuffer = CommandBuffer;
        this.RenderPass = RenderPass;
        this.Subpass = Subpass;
        this.SampleCount = SampleCount;
        this.ColorFormat = ColorFormat;
        this.DepthStencilFormat = DepthStencilFormat;
    }

    /// <summary>
    /// The device that the session renders on. Make the objects that your
    /// commands use on <c>context.device</c>. Don't submit to
    /// <c>context.graphics_queue</c> from a callback.
    /// </summary>
    public required VulkanContextDescriptor Context { get; init; }

    /// <summary>
    /// The frame's <c>VkCommandBuffer</c>, recording inside <c>render_pass</c>.
    /// </summary>
    public NativePointer CommandBuffer { get; init; }

    /// <summary>
    /// The <c>VkRenderPass</c> that <c>command_buffer</c> records in.
    /// </summary>
    public ulong RenderPass { get; init; }

    /// <summary>
    /// The index of the subpass that <c>command_buffer</c> records in.
    /// </summary>
    public uint Subpass { get; init; }

    /// <summary>
    /// The <c>VkSampleCountFlagBits</c> of the render pass's attachments.
    /// </summary>
    public uint SampleCount { get; init; }

    /// <summary>
    /// The <c>VkFormat</c> of the render pass's color attachment.
    /// </summary>
    public uint ColorFormat { get; init; }

    /// <summary>
    /// The <c>VkFormat</c> of the render pass's depth-stencil attachment, or
    /// <c>VK_FORMAT_UNDEFINED</c> when it has none.
    /// </summary>
    public uint DepthStencilFormat { get; init; }
}
