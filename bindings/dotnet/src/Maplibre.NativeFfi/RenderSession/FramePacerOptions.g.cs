// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for creating a frame pacer.
/// </summary>
/// <remarks>
/// See <c>mln_frame_pacer_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public sealed record FramePacerOptions
{
    /// <summary>
    /// The most times per second that the pacer demands frames, greater than
    /// zero. The pacer skips a display refresh that comes too soon after its
    /// last demands, and a limit at the display's refresh rate or above skips
    /// none. By default, it can demand frames at every refresh.
    /// </summary>
    public uint? MaximumFramesPerSecond { get; init; }
}
