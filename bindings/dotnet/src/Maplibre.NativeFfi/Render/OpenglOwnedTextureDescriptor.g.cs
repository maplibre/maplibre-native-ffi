// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct OpenglOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    OpenglContextDescriptor Context
)
{
    public static OpenglOwnedTextureDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_opengl_owned_texture_descriptor_default");
            return CopyOpenglOwnedTextureDescriptor(
                NativeMethods.mln_opengl_owned_texture_descriptor_default()
            );
        }
    }
}
