// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your Metal textures to render into, on the Metal device that you
/// attach on.
/// </summary>
/// <remarks>
/// <para><c>physical_width</c> and <c>physical_height</c> give the textures'
/// size in pixels, which is usually <c>logical_size</c> times the map's scale
/// factor. The textures must all have that size, the device's <c>MTLDevice</c>,
/// one pixel format, one sample, and <c>MTLTextureUsageRenderTarget</c>. The
/// ring holds up to <c>MaplibreNative.TextureRingMaxDepth</c> distinct
/// textures.</para>
/// <para>Give the ring <c>depth_textures</c> to draw into each frame
/// depth-tested against the map, as <c>FrameProjection</c> says: one for each
/// of <c>textures</c>, at the same index, or none. Each frame clears the one at
/// its slot, renders the map's depth and stencil into it, and stores them. They
/// must have the size of <c>textures</c>, one pixel format of
/// <c>GpuDeviceCapabilities.DepthStencilFormats</c>, one sample,
/// <c>MTLTextureUsageRenderTarget</c>, and a storage mode other than
/// <c>MTLStorageModeMemoryless</c>.</para>
/// <para>See <c>mln_metal_borrowed_textures_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct MetalBorrowedTexturesTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalBorrowedTexturesTarget(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<MetalBorrowedTexture> Textures,
        ImmutableArray<MetalBorrowedTexture> DepthTextures
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
        this.DepthTextures = DepthTextures;
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
    public ImmutableArray<MetalBorrowedTexture> DepthTextures
    {
        get => DepthTexturesStorage.AsImmutable();
        init => DepthTexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<MetalBorrowedTexture> DepthTexturesStorage { get; init; }
}
