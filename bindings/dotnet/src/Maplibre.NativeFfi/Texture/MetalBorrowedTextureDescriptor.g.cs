// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Metal attachment options for a borrowed texture target.
/// </summary>
/// <remarks>
/// See <c>mln_metal_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
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

    /// <summary>
    /// Logical texture extent. Required. physical_width and physical_height
    /// give the texture's size. A scale_factor that differs from the map's is
    /// accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Physical texture width in device pixels. Required.
    /// </summary>
    public required uint PhysicalWidth { get; init; }

    /// <summary>
    /// Physical texture height in device pixels. Required.
    /// </summary>
    public required uint PhysicalHeight { get; init; }

    /// <summary>
    /// The ring's textures, one per slot, in slot order. Required.
    /// </summary>
    public required ImmutableArray<MetalBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<MetalBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// <c>id&lt;MTLCommandQueue&gt;</c> / <c>MTL::CommandQueue*</c> on the
    /// textures' device that the session submits its work to, retained by the
    /// session. Optional; null makes the session create its own queue.
    /// <c>GpuSyncKind.QueueOrder</c> orders frames on this queue. A replacement
    /// names the queue the session attached with.
    /// </summary>
    public NativePointer CommandQueue { get; init; }
}
