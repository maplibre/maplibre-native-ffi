// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Render;

public readonly partial record struct MetalSurfaceDescriptor(
    RenderTargetExtent Extent,
    MetalContextDescriptor Context,
    NativePointer Layer
)
{
    public static MetalSurfaceDescriptor Default
    {
        get
        {
            using var call = Enter(null, "mln_metal_surface_descriptor_default");
            return CopyMetalSurfaceDescriptor(NativeMethods.mln_metal_surface_descriptor_default());
        }
    }
}
