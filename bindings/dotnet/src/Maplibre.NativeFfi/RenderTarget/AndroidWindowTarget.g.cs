// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An Android window to present to.
/// </summary>
/// <remarks>
/// <para>The session takes its own reference to the window, so you can release
/// yours once the call returns. When the platform destroys the window, such as
/// in <c>SurfaceHolder.Callback.surfaceDestroyed</c>, release the target with
/// <c>RenderSessionHandle.ReleaseTargetAsync()</c>, or give the session another
/// window with <c>RenderSessionHandle.SetTargetAsync()</c>, without waiting for
/// that to complete. A frame to the destroyed window finishes with
/// <c>RenderResult.TargetNotReady</c>, or with <c>RenderResult.Rendered</c> if
/// the window went away while the frame was presenting.</para>
/// <para>See <c>mln_android_window_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct AndroidWindowTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public AndroidWindowTarget(LogicalSize LogicalSize, NativePointer Window)
    {
        this.LogicalSize = LogicalSize;
        this.Window = Window;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>ANativeWindow*</c>.
    /// </summary>
    public required NativePointer Window { get; init; }
}
