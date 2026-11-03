// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A new Metal layer for
/// <c>RenderSessionHandle.SetMetalSurfaceTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>The session renders to it with the device that it attached with. It
/// uses the layer until a replacement or release of the target completes, a
/// detach or disposal of the session completes, or an abandon of it
/// returns.</para>
/// <para>See <c>mln_metal_surface_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct MetalSurfaceReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalSurfaceReplacement(LogicalSize LogicalSize, NativePointer Layer)
    {
        this.LogicalSize = LogicalSize;
        this.Layer = Layer;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your CAMetalLayer.
    /// </summary>
    public required NativePointer Layer { get; init; }
}
