// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public readonly partial record struct MetalOwnedTextureDescriptor(
    RenderTargetExtent Extent,
    MetalContextDescriptor Context
)
{
    public static MetalOwnedTextureDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_metal_owned_texture_descriptor_default");
            return CopyMetalOwnedTextureDescriptor(
                NativeMethods.mln_metal_owned_texture_descriptor_default()
            );
        }
    }
}
