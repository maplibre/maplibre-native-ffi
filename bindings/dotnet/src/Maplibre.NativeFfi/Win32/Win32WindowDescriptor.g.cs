// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A Win32 window to present to, for <c>MapHandle.AttachWin32Window()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_win32_window_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct Win32WindowDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public Win32WindowDescriptor(LogicalSize LogicalSize, NativePointer Window)
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
