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
public readonly record struct OpenglBorrowedTextureDescriptor
{
    public OpenglBorrowedTextureDescriptor()
    {
        this = Default;
    }

    public OpenglBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        OpenglContextDescriptor Context,
        OpenglBorrowedTexture[] Textures,
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
    /// Logical texture extent; physical_width and physical_height give the
    /// texture's size. A scale_factor that differs from the map's is accepted
    /// and logged as a warning.
    /// </summary>
    public LogicalExtent Extent { get; init; }

    /// <summary>
    /// Physical texture width in device pixels. Must be positive. Defaults to
    /// 256.
    /// </summary>
    public uint PhysicalWidth { get; init; }

    /// <summary>
    /// Physical texture height in device pixels. Must be positive. Defaults to
    /// 256.
    /// </summary>
    public uint PhysicalHeight { get; init; }

    /// <summary>
    /// Borrowed OpenGL context provider data. The textures must belong to this
    /// context or a context in the same share group.
    /// </summary>
    public OpenglContextDescriptor Context { get; init; }

    /// <summary>
    /// The ring's textures, one per slot, in slot order. Required.
    /// </summary>
    public OpenglBorrowedTexture[] Textures
    {
        get => TexturesStorage.ToArray();
        init => TexturesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<OpenglBorrowedTexture> TexturesStorage { get; init; }

    /// <summary>
    /// OpenGL texture target of every texture. Must be GL_TEXTURE_2D.
    /// </summary>
    public uint Target { get; init; }
    public static OpenglBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyOpenglBorrowedTextureDescriptor(
                NativeMethods.mln_opengl_borrowed_texture_descriptor_default()
            );
        }
    }
}
