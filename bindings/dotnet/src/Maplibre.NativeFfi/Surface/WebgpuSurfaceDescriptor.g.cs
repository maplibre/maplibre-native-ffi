// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public readonly partial record struct WebgpuSurfaceDescriptor(
    RenderTargetExtent Extent,
    WebgpuContextDescriptor Context,
    NativePointer Surface,
    uint Format
)
{
    public WebgpuSurfaceDescriptor()
        : this(new RenderTargetExtent(), default, default, default) { }

    public static WebgpuSurfaceDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_webgpu_surface_descriptor_default");
            return GeneratedValues.CopyWebgpuSurfaceDescriptor(
                NativeMethods.mln_webgpu_surface_descriptor_default()
            );
        }
    }
}
