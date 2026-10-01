// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct OpenglBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    OpenglContextDescriptor Context,
    uint Texture,
    uint Target
)
{
    public static OpenglBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_opengl_borrowed_texture_descriptor_default");
            return CopyOpenglBorrowedTextureDescriptor(
                NativeMethods.mln_opengl_borrowed_texture_descriptor_default()
            );
        }
    }
}
