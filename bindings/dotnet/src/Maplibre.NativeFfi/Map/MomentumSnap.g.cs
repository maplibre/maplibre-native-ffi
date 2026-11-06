// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a part settles after a gesture that changed it.
/// </summary>
/// <remarks>
/// <para>The targets are every <c>spacing</c> from <c>offset</c>, in the part's
/// unit. Bearing targets wrap around the circle. Once the part's glide ends, or
/// right away when it doesn't move, the part eases to the nearest target if it
/// is within <c>tolerance</c> of it, and otherwise stays where it is. A setting
/// whose bit isn't set takes the part's default, which <c>GestureMomentum</c>
/// lists.</para>
/// <para>See <c>mln_momentum_snap</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record MomentumSnap
{
    /// <summary>
    /// Positive.
    /// </summary>
    public double? Spacing { get; init; }

    /// <summary>
    /// Defaults to 0.
    /// </summary>
    public double? Offset { get; init; }

    /// <summary>
    /// Not negative.
    /// </summary>
    public double? Tolerance { get; init; }

    /// <summary>
    /// Zero settles without easing. Defaults to 300.
    /// </summary>
    public double? DurationMs { get; init; }
}
