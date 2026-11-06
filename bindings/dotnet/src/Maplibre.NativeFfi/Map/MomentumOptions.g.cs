// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How one part of a gesture keeps moving after its release.
/// </summary>
/// <remarks>
/// <para>The part starts at its release speed. Speeds and distances are in the
/// part's unit: logical pixels for pan, zoom levels for zoom, and degrees for
/// bearing and pitch. A setting whose bit isn't set takes the part's default,
/// which <c>GestureMomentum</c> lists. <c>min_speed</c> is finite and not
/// negative, and every other setting is finite and positive.</para>
/// <para>See <c>mln_momentum_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record MomentumOptions
{
    public MomentumDecay? Decay { get; init; }

    /// <summary>
    /// For <c>MomentumDecay.Friction</c>, how long the speed takes to halve.
    /// </summary>
    public double? HalfLifeMs { get; init; }

    /// <summary>
    /// For <c>MomentumDecay.Friction</c>, the speed at which it ends.
    /// </summary>
    public double? StopSpeed { get; init; }

    /// <summary>
    /// For <c>MomentumDecay.Spline</c>, Android's
    /// <c>ViewConfiguration.getScrollFriction()</c>. Defaults to 0.015.
    /// </summary>
    public double? ScrollFriction { get; init; }

    /// <summary>
    /// A slower release starts no momentum.
    /// </summary>
    public double? MinSpeed { get; init; }

    /// <summary>
    /// A faster release counts as this speed.
    /// </summary>
    public double? MaxSpeed { get; init; }

    /// <summary>
    /// The farthest the part moves. A release that would move farther starts
    /// slower, so it ends sooner.
    /// </summary>
    public double? MaxDistance { get; init; }
}
