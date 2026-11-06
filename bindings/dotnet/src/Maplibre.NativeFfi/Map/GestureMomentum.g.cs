// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a gesture keeps moving the camera after its last contact lifts.
/// </summary>
/// <remarks>
/// <para>A part moves only when its velocity is set, and each part slows on its
/// own. Every part defaults to <c>MomentumDecay.Friction</c> with a half-life
/// of 140 ms, and these other defaults:</para>
/// <list type="bullet">
/// <item><description>Pan stops at 10 logical pixels per second, needs a
/// release of at least 200, and counts a release faster than 10000 as
/// 10000.</description></item>
/// <item><description>Zoom stops at 0.01 levels per second and moves at most
/// 2.5 levels.</description></item>
/// <item><description>Bearing stops at 1 degree per second and needs a release
/// of at least 60.</description></item>
/// <item><description>Pitch stops at 1 degree per second and needs a release of
/// at least 5.</description></item>
/// </list>
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
    /// Where the bearing settles after a gesture that changed it, once the rest
    /// of the momentum ends. Without momentum, it settles at once. Without
    /// this, the bearing stays where the gesture leaves it.
    /// </summary>
    public BearingSnap? BearingSnap { get; init; }
}
