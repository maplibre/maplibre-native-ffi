// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An OpenHarmony window to present to.
/// </summary>
/// <remarks>
/// <para>The session takes its own reference to the window, so you can release
/// yours once the call returns. When the platform destroys the window, such as
/// in an <c>XComponent</c>'s <c>onSurfaceDestroyed</c> callback, release the
/// target or give the session another window without waiting, as for an Android
/// window.</para>
/// <para>See <c>mln_ohos_window_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OhosWindowTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OhosWindowTarget(LogicalSize LogicalSize, NativePointer Window)
    {
        this.LogicalSize = LogicalSize;
        this.Window = Window;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>OHNativeWindow*</c>.
    /// </summary>
    public required NativePointer Window { get; init; }
}
