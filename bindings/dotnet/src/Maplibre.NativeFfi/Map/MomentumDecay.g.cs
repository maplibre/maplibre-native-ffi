// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a part of a gesture's momentum slows from its release speed.
/// </summary>
/// <remarks>
/// See <c>mln_momentum_decay</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum MomentumDecay : uint
{
    /// <summary>
    /// The speed halves every <c>half_life_ms</c>, until it drops to
    /// <c>stop_speed</c>. UIScrollView's normal deceleration is a half-life of
    /// 346 ms, and its fast deceleration 69 ms.
    /// </summary>
    Friction = 0,

    /// <summary>
    /// Android's scroll fling, as OverScroller computes it with
    /// <c>scroll_friction</c>. Only pan can use it.
    /// </summary>
    Spline = 1,
}
