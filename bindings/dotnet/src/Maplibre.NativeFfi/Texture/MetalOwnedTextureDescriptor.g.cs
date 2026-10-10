// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct MetalOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    MetalContextDescriptor Context
)
{
    public static MetalOwnedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_metal_owned_texture_descriptor_default");
            return GeneratedValues.CopyMetalOwnedTextureDescriptor(
                NativeMethods.mln_metal_owned_texture_descriptor_default()
            );
        }
    }
}
