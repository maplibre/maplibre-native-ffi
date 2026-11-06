// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How one part of a gesture keeps moving after its release.
/// </summary>
/// <remarks>
/// <para>Speeds and distances are in the part's unit: logical pixels for pan,
/// zoom levels for zoom, and degrees for bearing and pitch. A setting whose bit
/// isn't set takes the part's default, which <c>GestureMomentum</c> lists.
/// Every setting is finite and not negative.</para>
/// <para>See <c>mln_momentum_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record MomentumOptions
{
    public MomentumDecay? Decay { get; init; }

    /// <summary>
    /// The momentum starts at this multiple of the release speed.
    /// </summary>
    public double? SpeedScale { get; init; }

    /// <summary>
    /// A slower release starts no momentum.
    /// </summary>
    public double? MinSpeed { get; init; }

    /// <summary>
    /// A faster release counts as this speed.
    /// </summary>
    public double? MaxSpeed { get; init; }

    /// <summary>
    /// For <c>MomentumDecay.Power</c>. At least 1.
    /// </summary>
    public double? Exponent { get; init; }

    /// <summary>
    /// For <c>MomentumDecay.Power</c>, the duration is <c>base_duration_ms</c>
    /// plus <c>duration_ms_per_speed</c> times the release speed.
    /// </summary>
    public double? BaseDurationMs { get; init; }
    public double? DurationMsPerSpeed { get; init; }

    /// <summary>
    /// Required for <c>MomentumDecay.Exponential</c>, and positive.
    /// </summary>
    public double? DecayRate { get; init; }

    /// <summary>
    /// Required for <c>MomentumDecay.Exponential</c>, and positive.
    /// </summary>
    public double? StopSpeed { get; init; }

    /// <summary>
    /// Stretches the momentum in time without changing its starting speed, so
    /// it also scales the distance. 0 starts no momentum. A host can pass its
    /// platform's animation duration scale here.
    /// </summary>
    public double? DurationScale { get; init; }

    /// <summary>
    /// The longest duration, after <c>duration_scale</c> applies.
    /// </summary>
    public double? MaxDurationMs { get; init; }

    /// <summary>
    /// The farthest the momentum moves. A momentum that would move farther
    /// slows faster over the same duration.
    /// </summary>
    public double? MaxDistance { get; init; }
}
