// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a part of a gesture's momentum slows from its release speed.
/// </summary>
/// <remarks>
/// See <c>mln_momentum_decay_model</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum MomentumDecayModel : uint
{
    /// <summary>
    /// The speed halves every <c>half_life_ms</c>, until it drops to
    /// <c>stop_speed</c>. UIScrollView's normal deceleration is a half-life of
    /// 346 ms, and its fast deceleration 69 ms.
    /// </summary>
    Friction = 0,

    /// <summary>
    /// The part moves <c>distance</c> over <c>duration_ms</c>, along
    /// <c>curve</c>. The caller plans the glide, for example from the
    /// platform's own fling physics, such as Android's OverScroller.
    /// </summary>
    Planned = 1,
}
