// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for creating a frame pacer, or for
/// <c>FramePacerHandle.SetOptions()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_frame_pacer_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public sealed record FramePacerOptions
{
    /// <summary>
    /// The most frames per second that the pacer demands, greater than zero.
    /// The pacer skips display refreshes to stay within it. By default, it can
    /// demand frames at every refresh.
    /// </summary>
    public uint? MaximumFramesPerSecond { get; init; }

    /// <summary>
    /// Removes the frame rate limit, so the pacer can demand frames at every
    /// display refresh, as it does by default.
    /// </summary>
    public bool UnlimitedFramesPerSecond { get; init; }
}
