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
