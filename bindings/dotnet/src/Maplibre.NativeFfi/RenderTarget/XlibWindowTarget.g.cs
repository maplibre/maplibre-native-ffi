// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An X11 window to present to, through Xlib.
/// </summary>
/// <remarks>
/// <para>Keep the display connected until the session's detach
/// completes.</para>
/// <para>See <c>mln_xlib_window_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct XlibWindowTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public XlibWindowTarget(LogicalSize LogicalSize, NativePointer Display, ulong Window)
    {
        this.LogicalSize = LogicalSize;
        this.Display = Display;
        this.Window = Window;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>Display*</c>.
    /// </summary>
    public required NativePointer Display { get; init; }

    /// <summary>
    /// Your <c>Window</c>, on <c>display</c>.
    /// </summary>
    public required ulong Window { get; init; }
}
