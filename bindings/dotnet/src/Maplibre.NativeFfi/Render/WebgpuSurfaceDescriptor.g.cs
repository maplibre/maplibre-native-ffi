// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public readonly partial record struct WebgpuSurfaceDescriptor(
    RenderTargetExtent Extent,
    WebgpuContextDescriptor Context,
    NativePointer Surface,
    uint Format
)
{
    public static WebgpuSurfaceDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_webgpu_surface_descriptor_default");
            return CopyWebgpuSurfaceDescriptor(
                NativeMethods.mln_webgpu_surface_descriptor_default()
            );
        }
    }
}
