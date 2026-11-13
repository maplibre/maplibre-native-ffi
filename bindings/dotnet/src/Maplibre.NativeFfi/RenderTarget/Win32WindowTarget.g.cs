// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Win32 window to present to.
/// </summary>
/// <remarks>
/// <para>On an OpenGL device, the session sets the window's pixel format, so
/// don't set one yourself. While the window is minimized, frame demands finish
/// with <c>RenderResult.TargetNotReady</c>.</para>
/// <para>See <c>mln_win32_window_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct Win32WindowTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public Win32WindowTarget(LogicalSize LogicalSize, NativePointer Window)
    {
        this.LogicalSize = LogicalSize;
        this.Window = Window;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>HWND</c>.
    /// </summary>
    public required NativePointer Window { get; init; }
}
