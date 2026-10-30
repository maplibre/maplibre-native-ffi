// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Vulkan attachment options for a borrowed texture target.
/// </summary>
/// <remarks>
/// See <c>mln_vulkan_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
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
        uint InitialLayout,
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
        this.InitialLayout = InitialLayout;
        this.FinalLayout = FinalLayout;
        this.Owner = Owner;
    }

    /// <summary>
    /// Logical texture extent. Required. physical_width and physical_height
    /// give the texture's size. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Physical image width in device pixels. Required.
    /// </summary>
    public required uint PhysicalWidth { get; init; }

    /// <summary>
    /// Physical image height in device pixels. Required.
    /// </summary>
    public required uint PhysicalHeight { get; init; }

    /// <summary>
    /// Borrowed Vulkan context. All handles are required.
    /// </summary>
    public required VulkanContextDescriptor Context { get; init; }

    /// <summary>
    /// The ring's images, one per slot, in slot order. Required.
    /// </summary>
    public required ImmutableArray<VulkanBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<VulkanBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// Backend-native VkFormat value of every image. Required, so
    /// VK_FORMAT_UNDEFINED is invalid.
    /// </summary>
    public required uint Format { get; init; }

    /// <summary>
    /// Backend-native VkImageLayout value expected at render-pass begin.
    /// </summary>
    public uint InitialLayout { get; init; }

    /// <summary>
    /// Backend-native VkImageLayout value left after rendering succeeds, such
    /// as VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL. Required, so
    /// VK_IMAGE_LAYOUT_UNDEFINED is invalid.
    /// </summary>
    public required uint FinalLayout { get; init; }

    /// <summary>
    /// The queue family that owns each image while the host holds its frame.
    /// Unless it is the graphics queue's family, the session releases a frame's
    /// image to it in final_layout, before the frame's producer
    /// synchronization, and takes the image back without a transfer before it
    /// renders into it again, so initial_layout must be
    /// VK_IMAGE_LAYOUT_UNDEFINED. A replacement keeps the owner the session
    /// attached with.
    /// </summary>
    public VulkanImageOwner Owner { get; init; }
}
