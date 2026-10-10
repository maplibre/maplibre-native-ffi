// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// WebGPU attachment options for a native surface.
/// </summary>
/// <remarks>
/// See <c>mln_webgpu_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
/// </remarks>
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
