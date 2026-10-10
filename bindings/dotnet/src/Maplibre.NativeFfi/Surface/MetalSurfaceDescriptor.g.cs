// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

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
            using var call = NativeCall.Enter(null, "mln_metal_surface_descriptor_default");
            return GeneratedValues.CopyMetalSurfaceDescriptor(
                NativeMethods.mln_metal_surface_descriptor_default()
            );
        }
    }
}
