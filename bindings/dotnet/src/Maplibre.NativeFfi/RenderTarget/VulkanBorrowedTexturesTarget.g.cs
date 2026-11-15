// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your Vulkan images to render into, on the Vulkan device that you
/// attach on.
/// </summary>
/// <remarks>
/// <para><c>physical_width</c> and <c>physical_height</c> give the images' size
/// in pixels, which is usually <c>logical_size</c> times the map's scale
/// factor. Each image must have that size, <c>format</c>, and
/// <c>VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT</c>, and belong to the device's
/// <c>VkDevice</c>. The ring holds up to
/// <c>MaplibreNative.TextureRingMaxDepth</c> distinct images.</para>
/// <para>Give the ring <c>depth_textures</c> to draw into each frame
/// depth-tested against the map, as <c>FrameProjection</c> says: one for each
/// of <c>textures</c>, at the same index, or none. Each frame clears the one at
/// its slot, renders the map's depth and stencil into it, stores them, and
/// leaves it in <c>depth_final_layout</c>, with <c>owner</c> as for the color
/// image. They must have the size of <c>textures</c>, <c>depth_format</c>, one
/// sample, and <c>VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT</c>, and their
/// views both the depth and the stencil aspect.</para>
/// <para>See <c>mln_vulkan_borrowed_textures_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanBorrowedTexturesTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanBorrowedTexturesTarget(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<VulkanBorrowedTexture> Textures,
        uint Format,
        uint FinalLayout,
        VulkanImageOwner Owner,
        ImmutableArray<VulkanBorrowedTexture> DepthTextures,
        uint DepthFormat,
        uint DepthFinalLayout
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
        this.Format = Format;
        this.FinalLayout = FinalLayout;
        this.Owner = Owner;
        this.DepthTextures = DepthTextures;
        this.DepthFormat = DepthFormat;
        this.DepthFinalLayout = DepthFinalLayout;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required ImmutableArray<VulkanBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<VulkanBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// The images' <c>VkFormat</c>.
    /// </summary>
    public required uint Format { get; init; }

    /// <summary>
    /// The <c>VkImageLayout</c> that each frame leaves its image in.
    /// </summary>
    public required uint FinalLayout { get; init; }
    public VulkanImageOwner Owner { get; init; }
    public ImmutableArray<VulkanBorrowedTexture> DepthTextures
    {
        get => DepthTexturesStorage.AsImmutable();
        init => DepthTexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<VulkanBorrowedTexture> DepthTexturesStorage { get; init; }

    /// <summary>
    /// The depth images' <c>VkFormat</c>, of
    /// <c>GpuDeviceCapabilities.DepthStencilFormats</c>.
    /// </summary>
    public uint DepthFormat { get; init; }

    /// <summary>
    /// The <c>VkImageLayout</c> that each frame leaves its depth image in.
    /// </summary>
    public uint DepthFinalLayout { get; init; }
}
