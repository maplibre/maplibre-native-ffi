// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A new Wayland surface for
/// <c>RenderSessionHandle.SetWaylandSurfaceTargetAsync()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_wayland_surface_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WaylandSurfaceReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WaylandSurfaceReplacement(LogicalSize LogicalSize, NativePointer Surface)
    {
        this.LogicalSize = LogicalSize;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>wl_surface*</c>, on the display that the session attached with.
    /// </summary>
    public required NativePointer Surface { get; init; }
}
