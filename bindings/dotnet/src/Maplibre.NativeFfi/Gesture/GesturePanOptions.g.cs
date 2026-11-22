// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How gestures move the map's content.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_pan_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public sealed record GesturePanOptions
{
    /// <summary>
    /// Whether drags, two fingers moving together, and deltas pan.
    /// </summary>
    public bool? Enabled { get; init; }

    /// <summary>
    /// Whether a release glides on.
    /// </summary>
    public bool? Glides { get; init; }
    public MomentumDecay? Momentum { get; init; }
}
