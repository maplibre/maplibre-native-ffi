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
/// <param name="Extent">
/// Logical texture extent. The map viewport uses width and height and the
/// renderer uses scale_factor; the physical size is stated separately below.
/// </param>
/// <param name="PhysicalWidth">
/// Physical texture width in device pixels. Must be positive. Defaults to 256.
/// </param>
/// <param name="PhysicalHeight">
/// Physical texture height in device pixels. Must be positive. Defaults to 256.
/// </param>
/// <param name="Context">
/// Borrowed OpenGL context provider data. The texture must belong to this
/// context or a context in the same share group.
/// </param>
/// <param name="Texture">
/// Borrowed OpenGL texture object name. Required.
/// </param>
/// <param name="Target">
/// OpenGL texture target. GL_TEXTURE_2D is the expected target.
/// </param>
public readonly partial record struct OpenglBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    OpenglContextDescriptor Context,
    uint Texture,
    uint Target
)
{
    public OpenglBorrowedTextureDescriptor()
        : this(new RenderTargetExtent(), 256, 256, default, default, default) { }

    public static OpenglBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(
                null,
                "mln_opengl_borrowed_texture_descriptor_default"
            );
            return GeneratedValues.CopyOpenglBorrowedTextureDescriptor(
                NativeMethods.mln_opengl_borrowed_texture_descriptor_default()
            );
        }
    }
}
