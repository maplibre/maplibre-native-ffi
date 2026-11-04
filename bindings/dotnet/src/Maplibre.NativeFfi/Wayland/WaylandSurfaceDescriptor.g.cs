// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Wayland surface to present to, for
/// <c>MapHandle.AttachWaylandSurface()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_wayland_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/wayland_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WaylandSurfaceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WaylandSurfaceDescriptor(
        LogicalSize LogicalSize,
        NativePointer Display,
        NativePointer Surface
    )
    {
        this.LogicalSize = LogicalSize;
        this.Display = Display;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>wl_display*</c>.
    /// </summary>
    public required NativePointer Display { get; init; }

    /// <summary>
    /// Your <c>wl_surface*</c>, on <c>display</c>.
    /// </summary>
    public required NativePointer Surface { get; init; }
}
