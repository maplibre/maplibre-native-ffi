// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your WebGPU textures to render into, on a WebGPU device that wraps
/// yours.
/// </summary>
/// <remarks>
/// <para><c>physical_width</c> and <c>physical_height</c> give the textures'
/// size in pixels, which is usually <c>logical_size</c> times the map's scale
/// factor. Each texture must be 2D with that size, <c>format</c>, one layer,
/// one mip level, one sample, and <c>GPUTextureUsage.RENDER_ATTACHMENT</c>. The
/// ring holds up to <c>MaplibreNative.TextureRingMaxDepth</c> distinct
/// textures.</para>
/// <para>Give the ring <c>depth_textures</c> to draw into each frame
/// depth-tested against the map, as <c>FrameProjection</c> says: one for each
/// of <c>textures</c>, at the same index, or none. Each frame clears the one at
/// its slot, renders the map's depth and stencil into it, and stores them. They
/// must be 2D with the size of <c>textures</c>, <c>depth_format</c>, one layer,
/// one mip level, one sample, and
/// <c>GPUTextureUsage.RENDER_ATTACHMENT</c>.</para>
/// <para>See <c>mln_web_gpu_borrowed_textures_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct WebGpuBorrowedTexturesTarget
{
    public WebGpuBorrowedTexturesTarget() { }

    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WebGpuBorrowedTexturesTarget(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<WebGpuBorrowedTexture> Textures,
        string Format,
        ImmutableArray<WebGpuBorrowedTexture> DepthTextures,
        string DepthFormat
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
        this.Format = Format;
        this.DepthTextures = DepthTextures;
        this.DepthFormat = DepthFormat;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required ImmutableArray<WebGpuBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<WebGpuBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// The textures' <c>GPUTextureFormat</c>, such as <c>rgba8unorm</c>.
    /// </summary>
    public required string Format { get; init; }
    public ImmutableArray<WebGpuBorrowedTexture> DepthTextures
    {
        get => DepthTexturesStorage.AsImmutable();
        init => DepthTexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<WebGpuBorrowedTexture> DepthTexturesStorage { get; init; }

    /// <summary>
    /// The depth textures' <c>GPUTextureFormat</c>, of
    /// <c>GpuDeviceCapabilities.DepthStencilFormats</c>.
    /// </summary>
    public string DepthFormat { get; init; } = "";
}
