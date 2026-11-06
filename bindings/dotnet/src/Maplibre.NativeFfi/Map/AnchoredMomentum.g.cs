// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a gesture's zoom, bearing, or pitch keeps changing after release.
/// </summary>
/// <remarks>
/// See <c>mln_anchored_momentum</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record AnchoredMomentum
{
    /// <summary>
    /// How fast the value changed at release: zoom levels per second for zoom,
    /// and degrees per second for bearing and pitch. The part moves only when
    /// this is set.
    /// </summary>
    public double? Velocity { get; init; }

    /// <summary>
    /// The screen point that the change keeps in place. Without one, the change
    /// keeps the center of the view in place.
    /// </summary>
    public ScreenPoint? Anchor { get; init; }
    public MomentumDecay Decay { get; init; } = new();
}
