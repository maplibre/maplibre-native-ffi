// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your Vulkan images, for
/// <c>MapHandle.AttachVulkanBorrowedTexture()</c> and
/// <c>RenderSessionHandle.SetVulkanBorrowedTextureTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para><c>extent</c> sets the map's size and the session's scale factor, so
/// give it the map's scale factor. <c>physical_width</c> and
/// <c>physical_height</c> give the images' size in pixels. Each image must have
/// that size, <c>format</c>, and VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, and
/// belong to the context's device.</para>
/// <para>The session uses the images until a replacement or release of the
/// target completes, a detach or disposal of the session completes, or an
/// abandon of it returns.</para>
/// <para>See <c>mln_vulkan_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanBorrowedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        VulkanContextDescriptor Context,
        ImmutableArray<VulkanBorrowedTexture> Textures,
        uint Format,
        uint FinalLayout,
        VulkanImageOwner Owner
    )
    {
        this.Extent = Extent;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Context = Context;
        this.Textures = Textures;
        this.Format = Format;
        this.FinalLayout = FinalLayout;
        this.Owner = Owner;
    }

    public required LogicalExtent Extent { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required VulkanContextDescriptor Context { get; init; }
    public required ImmutableArray<VulkanBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<VulkanBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// The images' VkFormat.
    /// </summary>
    public required uint Format { get; init; }

    /// <summary>
    /// The VkImageLayout that each frame leaves its image in.
    /// </summary>
    public required uint FinalLayout { get; init; }
    public VulkanImageOwner Owner { get; init; }
}
