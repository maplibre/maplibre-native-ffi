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
public readonly record struct VulkanBorrowedTextureDescriptor
{
    public VulkanBorrowedTextureDescriptor()
    {
        this = Default;
    }

    public VulkanBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        VulkanContextDescriptor Context,
        VulkanBorrowedTexture[] Textures,
        uint Format,
        uint InitialLayout,
        uint FinalLayout
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
    }

    /// <summary>
    /// Logical texture extent. The map viewport uses width and height and the
    /// renderer uses scale_factor; the physical size is stated separately
    /// below. A scale_factor that differs from the map's is accepted and logged
    /// as a warning.
    /// </summary>
    public LogicalExtent Extent { get; init; }

    /// <summary>
    /// Physical image width in device pixels. Must be positive. Defaults to
    /// 256.
    /// </summary>
    public uint PhysicalWidth { get; init; }

    /// <summary>
    /// Physical image height in device pixels. Must be positive. Defaults to
    /// 256.
    /// </summary>
    public uint PhysicalHeight { get; init; }

    /// <summary>
    /// Borrowed Vulkan context. All handles are required.
    /// </summary>
    public VulkanContextDescriptor Context { get; init; }

    /// <summary>
    /// The ring's images, one per slot, in slot order. Required.
    /// </summary>
    public VulkanBorrowedTexture[] Textures
    {
        get => TexturesStorage.ToArray();
        init => TexturesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<VulkanBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// Backend-native VkFormat value of every image. VK_FORMAT_UNDEFINED is
    /// invalid.
    /// </summary>
    public uint Format { get; init; }

    /// <summary>
    /// Backend-native VkImageLayout value expected at render-pass begin.
    /// </summary>
    public uint InitialLayout { get; init; }

    /// <summary>
    /// Backend-native VkImageLayout value left after rendering succeeds.
    /// Defaults to 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
    /// </summary>
    public uint FinalLayout { get; init; }
    public static VulkanBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyVulkanBorrowedTextureDescriptor(
                NativeMethods.mln_vulkan_borrowed_texture_descriptor_default()
            );
        }
    }
}
