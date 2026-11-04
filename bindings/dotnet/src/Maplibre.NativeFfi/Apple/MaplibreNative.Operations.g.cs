// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Creates a frame pacer that follows the display through a
    /// <c>CADisplayLink</c>.
    /// </summary>
    /// <remarks>
    /// <para>You can call it on any thread. The pacer needs no run loop of
    /// yours. On macOS, it follows the main display, the one with the menu
    /// bar.</para>
    /// <para>With <c>FramePacerOptions.MaximumFramesPerSecond</c>, the pacer
    /// also asks the system for that refresh rate. On iPhone, a rate above 60
    /// needs <c>CADisableMinimumFrameDurationOnPhone</c> in the app's
    /// <c>Info.plist</c>.</para>
    /// <para>See <c>mln_apple_frame_pacer_create</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/apple_8h.html">C API reference</see>.</para>
    /// <para>Errors:</para>
    /// <list type="bullet">
    /// <item><description><c>Status.NativeError</c> when the system has no
    /// display to follow.</description></item>
    /// </list>
    /// </remarks>
    [global::System.Runtime.Versioning.SupportedOSPlatform("ios")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("maccatalyst")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]
    [global::System.Runtime.Versioning.SupportedOSPlatform("tvos")]
    public static FramePacerHandle AppleFramePacerCreate(FramePacerOptions? options = null)
    {
        using var call = Enter(null);
        var nativeOptions = options is null
            ? default(mln_frame_pacer_options)
            : NativeFramePacerOptions(options);
        MlnFramePacer outPacer = default;
        Check(
            NativeMethods.mln_apple_frame_pacer_create(
                options is null ? null : &nativeOptions,
                &outPacer,
                Diagnostic
            )
        );
        return FramePacerHandle.Adopt(outPacer);
    }
}
