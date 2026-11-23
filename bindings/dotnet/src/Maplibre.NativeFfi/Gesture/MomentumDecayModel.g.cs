// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a part's glide unfolds from its release speed.
/// </summary>
/// <remarks>
/// See <c>mln_momentum_decay_model</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
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
    /// The glide lasts <c>duration_ms</c> × (speed / <c>reference_speed</c>) to
    /// the power of <c>duration_exponent</c>, and follows <c>easing</c>,
    /// starting at the release speed. Its defaults are Android's OverScroller
    /// fling, in its density-independent pixels: a reference speed of 1000,
    /// 555.182 ms, an exponent of 0.7362676, the curve (0.175, 0.5, 0.35, 1), a
    /// minimum speed of 50 and a maximum of 8000.
    /// </summary>
    Curve = 1,
}
