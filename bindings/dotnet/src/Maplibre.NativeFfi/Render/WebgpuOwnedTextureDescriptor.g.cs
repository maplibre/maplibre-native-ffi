// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

public readonly partial record struct WebgpuOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    WebgpuContextDescriptor Context
)
{
    public static WebgpuOwnedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_webgpu_owned_texture_descriptor_default");
            return GeneratedValues.CopyWebgpuOwnedTextureDescriptor(
                NativeMethods.mln_webgpu_owned_texture_descriptor_default()
            );
        }
    }
}
