// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Metal layer to render to, for <c>MapHandle.AttachMetalSurface()</c> and
/// <c>RenderSessionHandle.SetMetalSurfaceTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>The session sets the layer's device, pixel format, and drawable size.
/// It uses the layer until a replacement or release of the target completes, a
/// detach or disposal of the session completes, or an abandon of it
/// returns.</para>
/// <para>See <c>mln_metal_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
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

    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// The MTLDevice to render with. A null device picks the system's default.
    /// </summary>
    public MetalContextDescriptor Context { get; init; }

    /// <summary>
    /// Your CAMetalLayer.
    /// </summary>
    public required NativePointer Layer { get; init; }
}
