// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Limits on the camera, for <c>MapHandle.SetBoundsAsync()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_bound_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record BoundOptions
{
    /// <summary>
    /// The area that the camera's center stays within.
    /// </summary>
    public LatLngBounds? Bounds { get; init; }
    public double? MinZoom { get; init; }
    public double? MaxZoom { get; init; }

    /// <summary>
    /// The lowest pitch, in degrees.
    /// </summary>
    public double? MinPitch { get; init; }

    /// <summary>
    /// The highest pitch, in degrees.
    /// </summary>
    public double? MaxPitch { get; init; }

    /// <summary>
    /// Removes the area that limits the camera's center.
    /// </summary>
    public bool Unbounded { get; init; }
}
