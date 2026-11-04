// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of your OpenGL textures, for
/// <c>MapHandle.AttachOpenGlBorrowedTexture()</c>.
/// </summary>
/// <remarks>
/// <para><c>logical_size</c> sets the map's size. <c>physical_width</c> and
/// <c>physical_height</c> give the textures' size in pixels, which is usually
/// <c>logical_size</c> times the map's scale factor. The context must use
/// <c>OpenGlContextOwnership.Shared</c>. Each texture must be in your context's
/// share group, with storage of that size. The ring holds up to
/// <c>MaplibreNative.TextureRingMaxDepth</c> distinct textures.</para>
/// <para>The session uses the textures until a replacement or release of the
/// target completes, a detach or release of the session completes, or an
/// abandon of it returns.</para>
/// <para>See <c>mln_open_gl_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OpenGlBorrowedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlBorrowedTextureDescriptor(
        LogicalSize LogicalSize,
        uint PhysicalWidth,
        uint PhysicalHeight,
        OpenGlContextDescriptor Context,
        ImmutableArray<OpenGlBorrowedTexture> Textures,
        uint Target
    )
    {
        this.LogicalSize = LogicalSize;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Context = Context;
        this.Textures = Textures;
        this.Target = Target;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required uint PhysicalWidth { get; init; }
    public required uint PhysicalHeight { get; init; }
    public required OpenGlContextDescriptor Context { get; init; }
    public required ImmutableArray<OpenGlBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<OpenGlBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// The textures' target. It must be GL_TEXTURE_2D.
    /// </summary>
    public required uint Target { get; init; }
}
