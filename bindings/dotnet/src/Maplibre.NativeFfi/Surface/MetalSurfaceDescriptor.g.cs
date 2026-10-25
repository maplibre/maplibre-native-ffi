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
public readonly partial record struct MetalSurfaceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalSurfaceDescriptor(
        LogicalExtent Extent,
        MetalContextDescriptor Context,
        NativePointer Layer
    )
    {
        this.Extent = Extent;
        this.Context = Context;
        this.Layer = Layer;
    }

    /// <summary>
    /// Logical surface extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Metal backend context. device is optional for Metal surfaces.
    /// </summary>
    public MetalContextDescriptor Context { get; init; }

    /// <summary>
    /// <c>CAMetalLayer*</c> / <c>CA::MetalLayer*</c> retained by the session.
    /// Required.
    /// </summary>
    public required NativePointer Layer { get; init; }
}
