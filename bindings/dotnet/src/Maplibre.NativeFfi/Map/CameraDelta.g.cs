// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A change to the camera from where it is now, for gestures.
/// </summary>
/// <remarks>
/// See <c>mln_camera_delta</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record CameraDelta
{
    /// <summary>
    /// Moves the map's content this far on the screen, as a drag does.
    /// </summary>
    public ScreenPoint? Offset { get; init; }

    /// <summary>
    /// Multiplies the map's scale. 2 zooms in by one level.
    /// </summary>
    public double? Scale { get; init; }

    /// <summary>
    /// Degrees to add to the bearing.
    /// </summary>
    public double? Bearing { get; init; }

    /// <summary>
    /// Degrees to add to the pitch.
    /// </summary>
    public double? Pitch { get; init; }

    /// <summary>
    /// The screen point that <c>scale</c>, <c>bearing</c>, and <c>pitch</c>
    /// keep in place. It needs one of them, and it combines with <c>offset</c>
    /// only in a delta without a duration.
    /// </summary>
    public ScreenPoint? Anchor { get; init; }

    /// <summary>
    /// How the gesture keeps moving after this delta. Only a delta in the
    /// <c>GesturePhase.End</c> phase carries it.
    /// </summary>
    public GestureMomentum? Momentum { get; init; }
    public AnimationOptions Animation { get; init; } = new();
    public GesturePhase GesturePhase { get; init; }
}
