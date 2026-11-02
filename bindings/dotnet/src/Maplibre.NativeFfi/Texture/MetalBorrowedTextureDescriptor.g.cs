// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your Metal textures, for
/// <c>MapHandle.AttachMetalBorrowedTexture()</c> and
/// <c>RenderSessionHandle.SetMetalBorrowedTextureTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para><c>extent</c> sets the map's size and scale factor, and
/// <c>physical_width</c> and <c>physical_height</c> give the textures' size in
/// pixels. The textures must all have that size, one device, one pixel format,
/// one sample, and MTLTextureUsageRenderTarget.</para>
/// <para>The session uses the textures until a replacement or release of the
/// target completes, a detach or disposal of the session completes, or an
/// abandon of it returns.</para>
/// <para>See <c>mln_metal_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct MetalBorrowedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<MetalBorrowedTexture> Textures,
        NativePointer CommandQueue
    )
    {
        this.Extent = Extent;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
        this.CommandQueue = CommandQueue;
    }

    public required LogicalExtent Extent { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required ImmutableArray<MetalBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<MetalBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// Your MTLCommandQueue on the textures' device, or null for a queue of the
    /// session's own. With your queue, frames can use
    /// <c>GpuSyncKind.QueueOrder</c>.
    /// </summary>
    public NativePointer CommandQueue { get; init; }
}
