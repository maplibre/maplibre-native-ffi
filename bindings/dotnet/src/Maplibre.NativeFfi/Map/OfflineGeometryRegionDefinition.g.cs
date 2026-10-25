// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Geometry offline region definition.
/// </summary>
/// <remarks>
/// See <c>mln_offline_geometry_region_definition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="StyleUrl">
/// Style URL. Copied during region creation.
/// </param>
/// <param name="Geometry">
/// UTF-8 GeoJSON Geometry bytes. Required. Borrowed during region creation.
/// </param>
/// <param name="MaxZoom">
/// Maximum zoom. Positive infinity lets each tile source use its own maximum
/// zoom.
/// </param>
public readonly partial record struct OfflineGeometryRegionDefinition(
    string StyleUrl,
    string Geometry,
    double MinZoom,
    double MaxZoom,
    float PixelRatio,
    bool IncludeIdeographs
);
