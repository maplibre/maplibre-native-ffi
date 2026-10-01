// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct WebgpuBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    WebgpuContextDescriptor Context,
    NativePointer Texture,
    NativePointer TextureView,
    uint Format
)
{
    public static WebgpuBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_webgpu_borrowed_texture_descriptor_default");
            return CopyWebgpuBorrowedTextureDescriptor(
                NativeMethods.mln_webgpu_borrowed_texture_descriptor_default()
            );
        }
    }
}
