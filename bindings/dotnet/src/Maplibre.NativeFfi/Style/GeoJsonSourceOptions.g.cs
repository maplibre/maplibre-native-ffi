// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for a GeoJSON
/// [source](https://maplibre.org/maplibre-style-spec/sources/#geojson).
/// </summary>
/// <remarks>
/// <para>Zoom levels are whole numbers.</para>
/// <para>See <c>mln_geo_json_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record GeoJsonSourceOptions
{
    public double? MinZoom { get; init; }
    public double? MaxZoom { get; init; }
    public double? Tolerance { get; init; }
    public double? ClusterMaxZoom { get; init; }

    /// <summary>
    /// The <c>clusterProperties</c> object, or empty for none.
    /// </summary>
    public string ClusterProperties { get; init; } = "";
    public uint? Buffer { get; init; }
    public uint? ClusterRadius { get; init; }
    public uint? ClusterMinPoints { get; init; }
    public bool? LineMetrics { get; init; }
    public bool? Cluster { get; init; }

    /// <summary>
    /// Builds tiles while the map renders instead of in the background, so that
    /// new data shows in the next frame.
    /// </summary>
    public bool? SynchronousTiling { get; init; }
}
