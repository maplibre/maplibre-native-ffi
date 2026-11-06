// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a gesture keeps moving the camera after its last contact lifts.
/// </summary>
/// <remarks>
/// <para>A part moves only when its velocity is set, and each part slows on its
/// own clock. The parts' defaults are:</para>
/// <para>| Part | Exponent | Speed scale | Base ms | ms per speed | Other | |
/// ------- | -------- | ----------- | ------- | ------------ | ---------------
/// | | pan | 2 | 0.56 | 150 | 1/10.5 | min speed 250 | | zoom | 4 | 1 | 600 | 0
/// | max 600 ms, 2.5 | | bearing | 4 | 1 | 600 | 0 | max 600 ms | | pitch | 4 |
/// 1 | 300 | 0 | min speed 5 |</para>
/// <para>Every part defaults to <c>MomentumDecay.Power</c>, and the zoom moves
/// at most 2.5 levels.</para>
/// <para>See <c>mln_gesture_momentum</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record GestureMomentum
{
    /// <summary>
    /// How fast the map's content moved on the screen at release, in logical
    /// pixels per second.
    /// </summary>
    public ScreenPoint? PanVelocity { get; init; }

    /// <summary>
    /// In zoom levels per second.
    /// </summary>
    public double? ZoomVelocity { get; init; }

    /// <summary>
    /// In degrees per second.
    /// </summary>
    public double? BearingVelocity { get; init; }

    /// <summary>
    /// In degrees per second.
    /// </summary>
    public double? PitchVelocity { get; init; }

    /// <summary>
    /// The screen point that the zoom keeps in place. Without one, the zoom
    /// keeps the center of the view in place.
    /// </summary>
    public ScreenPoint? ZoomAnchor { get; init; }

    /// <summary>
    /// The screen point that the rotation keeps in place. Without one, the
    /// rotation turns around the center of the view.
    /// </summary>
    public ScreenPoint? BearingAnchor { get; init; }
    public MomentumOptions Pan { get; init; } = new();
    public MomentumOptions Zoom { get; init; } = new();
    public MomentumOptions Bearing { get; init; } = new();
    public MomentumOptions Pitch { get; init; } = new();

    /// <summary>
    /// When pan and zoom momentum both start, the pan slows over the zoom's
    /// duration and curve, starts no faster, and moves no farther than it would
    /// alone. Defaults to true.
    /// </summary>
    public bool? PanFollowsZoom { get; init; }

    /// <summary>
    /// Where the bearing settles after a gesture that changed it, once the rest
    /// of the momentum ends. Without momentum, it settles at once. Without
    /// this, the bearing stays where the gesture leaves it.
    /// </summary>
    public BearingSnap? BearingSnap { get; init; }
}
