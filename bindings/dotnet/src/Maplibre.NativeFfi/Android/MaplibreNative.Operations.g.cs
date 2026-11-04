// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Creates a frame pacer that follows the display through the calling
    /// thread's <c>AChoreographer</c>.
    /// </summary>
    /// <remarks>
    /// <para>Call it on a thread with an <c>ALooper</c>, such as the app's main
    /// thread, and keep that thread running its looper while sessions use the
    /// pacer.</para>
    /// <para>See <c>mln_android_frame_pacer_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.InvalidState</c> when the calling thread
    /// has no <c>ALooper</c>.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    public static FramePacerHandle AndroidFramePacerCreate(FramePacerOptions? options = null)
    {
        using var call = Enter(null);
        var nativeOptions = options is null
            ? default(mln_frame_pacer_options)
            : NativeFramePacerOptions(options);
        MlnFramePacer outPacer = default;
        Check(
            NativeMethods.mln_android_frame_pacer_create(
                options is null ? null : &nativeOptions,
                &outPacer,
                Diagnostic
            )
        );
        return FramePacerHandle.Adopt(outPacer);
    }

    /// <summary>
    /// Gives the library the Java VM and the app context that it needs on
    /// Android.
    /// </summary>
    /// <remarks>
    /// <para>Call it before creating a runtime. <c>jniEnv</c> is the calling
    /// thread's <c>JNIEnv*</c>, and <c>context</c> is an Android
    /// <c>Context</c>, such as the application.</para>
    /// <para>See <c>mln_android_init</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html">C API reference</see>.</para>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("android")]
    public static void InitAndroid(NativePointer jniEnv, NativePointer context)
    {
        using var call = Enter(null);
        Check(
            NativeMethods.mln_android_init(
                (void*)jniEnv.Address,
                (void*)context.Address,
                Diagnostic
            )
        );
    }
}
