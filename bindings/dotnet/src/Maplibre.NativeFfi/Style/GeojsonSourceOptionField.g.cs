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
    Buffer = 32,
    ClusterRadius = 64,
    ClusterMinPoints = 128,
    LineMetrics = 256,
    Cluster = 512,
    SynchronousTiling = 1024,
}
