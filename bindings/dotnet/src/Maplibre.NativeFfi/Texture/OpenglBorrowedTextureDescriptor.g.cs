// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

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
