// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

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
