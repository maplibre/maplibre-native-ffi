// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// All the limits on a map's camera.
/// </summary>
/// <remarks>
/// <para>Its members mean the same as those of <c>BoundOptions</c>.</para>
/// <para>See <c>mln_camera_bounds</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Bounds">
/// The area that the camera's center stays within. When <c>unbounded</c> is
/// true, it covers the whole world.
/// </param>
/// <param name="Unbounded">
/// Whether no area limits the camera's center.
/// </param>
/// <param name="MinPitch">
/// The lowest pitch, in degrees.
/// </param>
/// <param name="MaxPitch">
/// The highest pitch, in degrees.
/// </param>
public readonly partial record struct CameraBounds(
    LatLngBounds Bounds,
    bool Unbounded,
    double MinZoom,
    double MaxZoom,
    double MinPitch,
    double MaxPitch
);
