// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Wayland surface to present to.
/// </summary>
/// <remarks>
/// <para>Keep the display connected until the session's detach completes. On an
/// OpenGL device, if you also use EGL on the display, initialize it before you
/// attach.</para>
/// <para>See <c>mln_wayland_surface_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct WaylandSurfaceTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WaylandSurfaceTarget(
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
