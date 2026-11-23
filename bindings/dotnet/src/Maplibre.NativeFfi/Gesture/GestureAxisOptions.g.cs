// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How gestures move the zoom, the bearing, or the pitch.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_axis_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public sealed record GestureAxisOptions
{
    /// <summary>
    /// Whether gestures move the part.
    /// </summary>
    public bool? Enabled { get; init; }

    /// <summary>
    /// Whether a release glides on.
    /// </summary>
    public bool? Glides { get; init; }
    public MomentumDecay? Momentum { get; init; }

    /// <summary>
    /// Whether the part settles once its glide ends.
    /// </summary>
    public bool? Snaps { get; init; }
    public MomentumSnap? Snap { get; init; }

    /// <summary>
    /// Where the part reports notches while gestures, their glides, and their
    /// snaps move it, replacing the ones it had. Several notches at one value
    /// report one event, with the kind of the first in the list.
    /// </summary>
    public ImmutableArray<GestureNotch>? Notches
    {
        get => NotchesStorage?.AsImmutable();
        init => NotchesStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<GestureNotch>? NotchesStorage { get; init; }
}
