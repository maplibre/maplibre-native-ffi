// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How one part of a gesture glides after release.
/// </summary>
/// <remarks>
/// <para>Speeds and distances are in the part's unit: logical pixels for pan,
/// zoom levels for zoom, and degrees for bearing and pitch, with speeds per
/// second. A setting whose bit isn't set keeps its current value. A new
/// <c>model</c> starts from its defaults: friction from the part's, which
/// <c>MapHandle.GestureRecognizerCreate()</c> lists, and a curve from Android's
/// fling. Settings of the other model are an error. <c>min_speed</c> is finite
/// and not negative, <c>duration_exponent</c> finite and above -1, and every
/// other number positive; the maximums may be infinite.</para>
/// <para>See <c>mln_momentum_decay</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record MomentumDecay
{
    public MomentumDecayModel? Model { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Friction</c>.
    /// </summary>
    public double? HalfLifeMs { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Friction</c>, the speed at which it ends.
    /// </summary>
    public double? StopSpeed { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Curve</c>, the speed that lasts
    /// <c>duration_ms</c>.
    /// </summary>
    public double? ReferenceSpeed { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Curve</c>.
    /// </summary>
    public double? DurationMs { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Curve</c>, how the duration grows with the
    /// speed.
    /// </summary>
    public double? DurationExponent { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Curve</c>, the share of the glide covered over
    /// the share of its duration. Its control points are between 0 and 1, and
    /// <c>x1</c> and <c>y1</c> above 0, so the glide starts moving and never
    /// passes its end.
    /// </summary>
    public UnitBezier? Easing { get; init; }

    /// <summary>
    /// A slower release doesn't glide.
    /// </summary>
    public double? MinSpeed { get; init; }

    /// <summary>
    /// A faster release counts as this.
    /// </summary>
    public double? MaxSpeed { get; init; }

    /// <summary>
    /// The farthest the part glides. A release that would glide farther counts
    /// as the speed that glides this far.
    /// </summary>
    public double? MaxDistance { get; init; }
}
