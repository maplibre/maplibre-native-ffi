// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A new Android window for
/// <c>RenderSessionHandle.SetAndroidSurfaceTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>Name the window by <c>window</c>, or by <c>jni_env</c> and
/// <c>surface</c>. The session takes its own reference to the window, so you
/// can release yours once the call returns. It uses the window until a
/// replacement or release of the target completes, a detach or release of the
/// session completes, or an abandon of it returns.</para>
/// <para>See <c>mln_android_surface_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct AndroidSurfaceReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public AndroidSurfaceReplacement(
        LogicalSize LogicalSize,
        NativePointer Window,
        NativePointer JniEnv,
        NativePointer Surface
    )
    {
        this.LogicalSize = LogicalSize;
        this.Window = Window;
        this.JniEnv = JniEnv;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>ANativeWindow*</c>, or null to name the window by
    /// <c>surface</c>.
    /// </summary>
    public NativePointer Window { get; init; }

    /// <summary>
    /// The calling thread's <c>JNIEnv*</c>, with <c>surface</c>.
    /// </summary>
    public NativePointer JniEnv { get; init; }

    /// <summary>
    /// An <c>android.view.Surface</c>, with <c>jni_env</c>.
    /// </summary>
    public NativePointer Surface { get; init; }
}
