// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a part of a gesture's momentum slows to a stop.
/// </summary>
/// <remarks>
/// See <c>mln_momentum_decay</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum MomentumDecay : uint
{
    /// <summary>
    /// The share of the distance traveled is 1 - (1 - t)^<c>exponent</c>, where
    /// t goes from 0 to 1 over the momentum's duration. An exponent of 2 slows
    /// at a constant rate.
    /// </summary>
    Power = 0,

    /// <summary>
    /// The speed falls to e^-<c>decay_rate</c> of itself each second, until it
    /// drops below <c>stop_speed</c>.
    /// </summary>
    Exponential = 1,
}
