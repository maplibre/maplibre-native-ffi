// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Where the bearing settles after a gesture that changed it.
/// </summary>
/// <remarks>
/// <para>The targets are <c>target_count</c> bearings spaced evenly around the
/// circle, starting at <c>target_offset</c>. The bearing eases to the nearest
/// one when it is within <c>tolerance</c> of it, and otherwise stays where it
/// is.</para>
/// <para>See <c>mln_bearing_snap</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record BearingSnap
{
    /// <summary>
    /// At least 1. Defaults to 1.
    /// </summary>
    public uint? TargetCount { get; init; }

    /// <summary>
    /// In degrees. Defaults to 0, north.
    /// </summary>
    public double? TargetOffset { get; init; }

    /// <summary>
    /// In degrees, from 0 to 180. Defaults to 7.
    /// </summary>
    public double? Tolerance { get; init; }

    /// <summary>
    /// Zero settles at once. Defaults to 300.
    /// </summary>
    public double? DurationMs { get; init; }
}
