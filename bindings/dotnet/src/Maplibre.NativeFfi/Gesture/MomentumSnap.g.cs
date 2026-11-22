// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a part settles after a gesture that changed it.
/// </summary>
/// <remarks>
/// <para>The targets are every <c>spacing</c> from <c>offset</c>, in the part's
/// unit. Bearing targets lie within one turn from <c>offset</c>, and the
/// bearing takes the shorter way around to the nearest. Once the part's glide
/// ends, or right away when it has none, the part eases to the nearest target
/// if it is within <c>tolerance</c> of it, and otherwise stays where it is. A
/// setting whose bit isn't set keeps its current value.</para>
/// <para>See <c>mln_momentum_snap</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record MomentumSnap
{
    /// <summary>
    /// Positive.
    /// </summary>
    public double? Spacing { get; init; }

    /// <summary>
    /// Finite.
    /// </summary>
    public double? Offset { get; init; }

    /// <summary>
    /// Not negative.
    /// </summary>
    public double? Tolerance { get; init; }

    /// <summary>
    /// Zero settles without easing.
    /// </summary>
    public double? DurationMs { get; init; }
}
