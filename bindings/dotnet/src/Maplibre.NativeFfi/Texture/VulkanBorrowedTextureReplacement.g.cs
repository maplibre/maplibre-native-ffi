// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// New Vulkan images for a borrowed ring, for
/// <c>RenderSessionHandle.SetVulkanBorrowedTextureTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>The members and the images follow the rules of
/// <c>VulkanBorrowedTextureDescriptor</c>. The ring keeps its depth, context,
/// <c>format</c>, <c>final_layout</c>, and <c>owner</c>, so give it as many
/// images as it attached with, in that format, on the same device.</para>
/// <para>See <c>mln_vulkan_borrowed_texture_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct VulkanBorrowedTextureReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public VulkanBorrowedTextureReplacement(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<VulkanBorrowedTexture> Textures
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
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
}
