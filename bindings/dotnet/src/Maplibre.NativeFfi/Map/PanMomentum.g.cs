// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How a gesture's pan keeps moving the map's content after release.
/// </summary>
/// <remarks>
/// See <c>mln_pan_momentum</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record PanMomentum
{
    /// <summary>
    /// How fast the map's content moved on the screen at release, in logical
    /// pixels per second. The pan moves only when this is set.
    /// </summary>
    public ScreenPoint? Velocity { get; init; }
    public MomentumDecay Decay { get; init; } = new();
}
