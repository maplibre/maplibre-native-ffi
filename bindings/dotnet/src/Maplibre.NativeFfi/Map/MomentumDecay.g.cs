// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How one part of a gesture's momentum slows from its release speed.
/// </summary>
/// <remarks>
/// <para>Speeds and distances are in the part's unit: logical pixels for pan,
/// zoom levels for zoom, and degrees for bearing and pitch. A setting whose bit
/// isn't set takes the part's default, which <c>GestureMomentum</c> lists.
/// <c>min_speed</c> is finite and not negative, and every other number is
/// finite and positive. A friction decay can serve every release, as a preset
/// that matches a platform's scrolling. An eased glide takes its direction from
/// the part's velocity, and its distance and duration from the caller's
/// plan.</para>
/// <para>See <c>mln_momentum_decay</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record MomentumDecay
{
    public MomentumDecayModel? Model { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Friction</c>, how long the speed takes to
    /// halve.
    /// </summary>
    public double? HalfLifeMs { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Friction</c>, the speed at which it ends.
    /// </summary>
    public double? StopSpeed { get; init; }

    /// <summary>
    /// Required for <c>MomentumDecayModel.Ease</c>.
    /// </summary>
    public double? Distance { get; init; }

    /// <summary>
    /// Required for <c>MomentumDecayModel.Ease</c>.
    /// </summary>
    public double? DurationMs { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Ease</c>, the share of <c>distance</c> covered
    /// over the share of <c>duration_ms</c>. Its control points are between 0
    /// and 1, so the glide never passes its distance. Defaults to (0, 0, 0.25,
    /// 1), the curve that camera animations ease along by default. Android's
    /// scroll fling follows (0.175, 0.5, 0.35, 1).
    /// </summary>
    public UnitBezier? Easing { get; init; }

    /// <summary>
    /// A slower release starts no momentum.
    /// </summary>
    public double? MinSpeed { get; init; }

    /// <summary>
    /// For <c>MomentumDecayModel.Friction</c>, a faster release counts as this.
    /// </summary>
    public double? MaxSpeed { get; init; }

    /// <summary>
    /// The farthest the part moves. A friction release that would move farther
    /// starts slower, so it ends sooner, and a planned glide moves at most this
    /// far over its duration.
    /// </summary>
    public double? MaxDistance { get; init; }
}
