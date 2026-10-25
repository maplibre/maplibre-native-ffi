// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL attachment options for a borrowed texture target.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OpenglBorrowedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenglBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        OpenglContextDescriptor Context,
        ImmutableArray<OpenglBorrowedTexture> Textures,
        uint Target
    )
    {
        this.Extent = Extent;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Context = Context;
        this.Textures = Textures;
        this.Target = Target;
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
    /// Borrowed OpenGL context provider data. The textures must belong to this
    /// context or a context in the same share group.
    /// </summary>
    public required OpenglContextDescriptor Context { get; init; }

    /// <summary>
    /// The ring's textures, one per slot, in slot order. Required.
    /// </summary>
    public required ImmutableArray<OpenglBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<OpenglBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// OpenGL texture target of every texture. Required: GL_TEXTURE_2D.
    /// </summary>
    public required uint Target { get; init; }
}
