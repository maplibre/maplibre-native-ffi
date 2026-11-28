// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The height of the ground at a coordinate.
/// </summary>
/// <remarks>
/// See <c>mln_map_projection_terrain_elevation_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
/// </remarks>
/// <param name="HasElevation">
/// Whether the map drew terrain at the coordinate.
/// </param>
/// <param name="ElevationMeters">
/// Meters above sea level, with the terrain's exaggeration. 0 without
/// <c>has_elevation</c>.
/// </param>
public readonly partial record struct MapProjectionTerrainElevationResult(
    bool HasElevation,
    double ElevationMeters
);
