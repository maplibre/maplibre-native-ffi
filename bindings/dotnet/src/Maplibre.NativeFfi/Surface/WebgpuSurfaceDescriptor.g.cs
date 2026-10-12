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
/// <param name="Extent">
/// Logical surface extent. A scale_factor that differs from the map's is
/// accepted and logged as a warning.
/// </param>
/// <param name="Context">
/// Borrowed WebGPU context. device is required.
/// </param>
/// <param name="Surface">
/// Borrowed WGPUSurface. Required, and must stay alive for the session. The
/// session configures it for this device and extent, and unconfigures it when
/// the session ends.
/// </param>
/// <param name="Format">
/// WGPUTextureFormat to configure the surface with. Required. A browser host
/// takes it from navigator.gpu.getPreferredCanvasFormat().
/// </param>
public readonly partial record struct WebgpuSurfaceDescriptor(
    LogicalExtent Extent,
    WebgpuContextDescriptor Context,
    NativePointer Surface,
    uint Format
)
{
    public WebgpuSurfaceDescriptor()
        : this(new LogicalExtent(), default, default, default) { }

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
