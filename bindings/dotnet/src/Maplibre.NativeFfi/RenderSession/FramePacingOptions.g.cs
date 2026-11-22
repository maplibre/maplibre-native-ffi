// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for <c>RenderSessionHandle.SetFramePacing()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_frame_pacing_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public sealed record FramePacingOptions
{
    /// <summary>
    /// The most frames per second that the library demands, greater than zero.
    /// It skips display refreshes to stay within it. By default, it can demand
    /// a frame at every refresh.
    /// </summary>
    public uint? MaximumFramesPerSecond { get; init; }

    /// <summary>
    /// The <c>NSView*</c> on macOS, or the <c>UIView*</c> on iOS and tvOS, that
    /// shows the session's frames, to pace them to its window's display rather
    /// than the main one. On iOS and tvOS, set the pacing again when the scene
    /// moves to another screen, as
    /// <c>windowScene:didUpdateEffectiveGeometry:</c> reports.
    /// </summary>
    public NativePointer? View { get; init; }
}
