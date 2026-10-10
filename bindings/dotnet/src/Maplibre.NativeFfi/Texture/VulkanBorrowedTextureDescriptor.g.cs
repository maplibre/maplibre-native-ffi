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
        : this(new LogicalExtent(), 256, 256, default, default!, default, default, 5) { }

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

    public LogicalExtent Extent { get; init; }
    public uint PhysicalWidth { get; init; }
    public uint PhysicalHeight { get; init; }
    public VulkanContextDescriptor Context { get; init; }
    public VulkanBorrowedTexture[] Textures
    {
        get => TexturesStorage.ToArray();
        init => TexturesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<VulkanBorrowedTexture> TexturesStorage { get; init; }
    public uint Format { get; init; }
    public uint InitialLayout { get; init; }
    public uint FinalLayout { get; init; }
    public static VulkanBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(
                null,
                "mln_vulkan_borrowed_texture_descriptor_default"
            );
            return GeneratedValues.CopyVulkanBorrowedTextureDescriptor(
                NativeMethods.mln_vulkan_borrowed_texture_descriptor_default()
            );
        }
    }
}
