// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your D3D12 textures to render into, on the D3D12 device that you
/// attach on.
/// </summary>
/// <remarks>
/// <para><c>physical_width</c> and <c>physical_height</c> give the textures'
/// size in pixels, which is usually <c>logical_size</c> times the map's scale
/// factor. Each texture must be a 2D resource of the device with that size, one
/// mip level, one array slice, one sample,
/// <c>D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET</c>, and one typed color format
/// that the device renders into, such as DXGI_FORMAT_R8G8B8A8_UNORM or
/// DXGI_FORMAT_B8G8R8A8_UNORM. The ring holds up to
/// <c>MaplibreNative.TextureRingMaxDepth</c> distinct textures, and references
/// each.</para>
/// <para>Each texture is in <c>state</c> when the session has it, and each
/// frame leaves it there.</para>
/// <para>Give the ring <c>depth_textures</c> to draw into each frame
/// depth-tested against the map, as <c>FrameProjection</c> says: one for each
/// of <c>textures</c>, at the same index, or none. Each frame clears the one at
/// its slot, renders the map's depth and stencil into it, stores them, and
/// leaves it in <c>depth_state</c>. They must have the size of <c>textures</c>,
/// one format of <c>GpuDeviceCapabilities.DepthStencilFormats</c>
/// (DXGI_FORMAT_D24_UNORM_S8_UINT or DXGI_FORMAT_D32_FLOAT_S8X24_UINT), one
/// sample, and <c>D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL</c>.</para>
/// <para>See <c>mln_d3d12_borrowed_textures_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct D3d12BorrowedTexturesTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public D3d12BorrowedTexturesTarget(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<D3d12BorrowedTexture> Textures,
        uint State,
        ImmutableArray<D3d12BorrowedTexture> DepthTextures,
        uint DepthState
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
        this.State = State;
        this.DepthTextures = DepthTextures;
        this.DepthState = DepthState;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required ImmutableArray<D3d12BorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<D3d12BorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// The textures' <c>D3D12_RESOURCE_STATES</c>.
    /// </summary>
    public uint State { get; init; }
    public ImmutableArray<D3d12BorrowedTexture> DepthTextures
    {
        get => DepthTexturesStorage.AsImmutable();
        init => DepthTexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<D3d12BorrowedTexture> DepthTexturesStorage { get; init; }

    /// <summary>
    /// The depth textures' <c>D3D12_RESOURCE_STATES</c>.
    /// </summary>
    public uint DepthState { get; init; }
}
