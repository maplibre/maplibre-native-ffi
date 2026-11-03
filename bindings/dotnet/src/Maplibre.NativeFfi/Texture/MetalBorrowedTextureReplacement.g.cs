// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// New Metal textures for a borrowed ring, for
/// <c>RenderSessionHandle.SetMetalBorrowedTextureTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>The members mean what they mean in
/// <c>MetalBorrowedTextureDescriptor</c>, and the textures follow its rules.
/// The ring keeps its depth, device, and command queue, so give it as many
/// textures as it attached with, on the same device. The session uses the
/// textures until a replacement or release of the target completes, a detach or
/// release of the session completes, or an abandon of it returns.</para>
/// <para>See <c>mln_metal_borrowed_texture_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct MetalBorrowedTextureReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalBorrowedTextureReplacement(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<MetalBorrowedTexture> Textures
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
    public required ImmutableArray<MetalBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<MetalBorrowedTexture> TexturesStorage { get; init; }
}
