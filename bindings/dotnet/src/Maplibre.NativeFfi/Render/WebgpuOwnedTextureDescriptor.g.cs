// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

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
            using var call = Enter(null, "mln_webgpu_owned_texture_descriptor_default");
            return CopyWebgpuOwnedTextureDescriptor(
                NativeMethods.mln_webgpu_owned_texture_descriptor_default()
            );
        }
    }
}
