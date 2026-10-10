// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Metal attachment options for a native surface.
/// </summary>
/// <remarks>
/// See <c>mln_metal_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// Logical surface extent. A scale_factor that differs from the map's is
/// accepted and logged as a warning.
/// </param>
/// <param name="Context">
/// Metal backend context. device is optional for Metal surfaces.
/// </param>
/// <param name="Layer">
/// <c>CAMetalLayer*</c> / <c>CA::MetalLayer*</c> retained by the session.
/// Required.
/// </param>
public readonly partial record struct MetalSurfaceDescriptor(
    LogicalExtent Extent,
    MetalContextDescriptor Context,
    NativePointer Layer
)
{
    public MetalSurfaceDescriptor()
        : this(new LogicalExtent(), default, default) { }

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
