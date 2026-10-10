// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_geojson_source_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_geojson_source_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum GeojsonSourceOptionField : uint
{
    MinZoom = 1,
    MaxZoom = 2,
    Tolerance = 4,
    ClusterMaxZoom = 8,
    ClusterProperties = 16,
    TileSize = 32,
    Buffer = 64,
    ClusterRadius = 128,
    ClusterMinPoints = 256,
    LineMetrics = 512,
    Cluster = 1024,
    SynchronousTiling = 2048,
}
