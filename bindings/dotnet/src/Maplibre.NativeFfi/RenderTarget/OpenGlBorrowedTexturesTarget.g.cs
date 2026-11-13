// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your OpenGL textures to render into, on an OpenGL device that
/// <c>GpuDeviceHandle.WrapOpenGl()</c> made from your context.
/// </summary>
/// <remarks>
/// <para><c>physical_width</c> and <c>physical_height</c> give the textures'
/// size in pixels, which is usually <c>logical_size</c> times the map's scale
/// factor. Each texture must be in your context's share group, with RGBA8
/// storage of that size. The ring holds up to
/// <c>MaplibreNative.TextureRingMaxDepth</c> distinct textures.</para>
/// <para>Give the ring <c>depth_textures</c> to draw into each frame
/// depth-tested against the map, as <c>FrameProjection</c> says: one for each
/// of <c>textures</c>, at the same index, or none. Each frame clears the one at
/// its slot, and renders the map's depth and stencil into it. They must be
/// <c>GL_TEXTURE_2D</c> textures in the share group, with storage of the size
/// of <c>textures</c> in a format of
/// <c>GpuDeviceCapabilities.DepthStencilFormats</c>.</para>
/// <para>See <c>mln_open_gl_borrowed_textures_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OpenGlBorrowedTexturesTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlBorrowedTexturesTarget(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<OpenGlBorrowedTexture> Textures,
        uint Target,
        ImmutableArray<OpenGlBorrowedTexture> DepthTextures
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
        this.Target = Target;
        this.DepthTextures = DepthTextures;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required ImmutableArray<OpenGlBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<OpenGlBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// The textures' target. It must be <c>GL_TEXTURE_2D</c>.
    /// </summary>
    public required uint Target { get; init; }
    public ImmutableArray<OpenGlBorrowedTexture> DepthTextures
    {
        get => DepthTexturesStorage.AsImmutable();
        init => DepthTexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<OpenGlBorrowedTexture> DepthTexturesStorage { get; init; }
}
