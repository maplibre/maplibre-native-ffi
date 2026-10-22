// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for GeoJSON sources, indexed and rendered at a fixed 512-pixel tile
/// size.
/// </summary>
/// <remarks>
/// See <c>mln_geojson_source_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record GeojsonSourceOptions
{
    /// <summary>
    /// Minimum tiling zoom. Defaults to 0.
    /// </summary>
    public double? MinZoom { get; set; }

    /// <summary>
    /// Maximum tiling zoom, an integer within [0, 24]. Defaults to 18.
    /// </summary>
    public double? MaxZoom { get; set; }

    /// <summary>
    /// Douglas-Peucker simplification tolerance. Defaults to 0.375.
    /// </summary>
    public double? Tolerance { get; set; }

    /// <summary>
    /// Highest zoom that clusters points. Defaults to 17.
    /// </summary>
    public double? ClusterMaxZoom { get; set; }

    /// <summary>
    /// Cluster aggregation expressions keyed by property name, as a JSON object
    /// whose members follow the MapLibre Style Spec clusterProperties form. The
    /// UTF-8 bytes are borrowed for the call.
    /// </summary>
    public byte[]? ClusterProperties
    {
        get => ClusterPropertiesStorage?.ToArray();
        set => ClusterPropertiesStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? ClusterPropertiesStorage { get; set; }

    /// <summary>
    /// Tile buffer in pixels within [0, 512]. Defaults to 128.
    /// </summary>
    public uint? Buffer { get; set; }

    /// <summary>
    /// Cluster radius in pixels within [0, 4095]. Defaults to 50.
    /// </summary>
    public uint? ClusterRadius { get; set; }

    /// <summary>
    /// Points required to form a cluster. Defaults to 2.
    /// </summary>
    public uint? ClusterMinPoints { get; set; }

    /// <summary>
    /// Adds line distance metrics to line features. Defaults to false.
    /// </summary>
    public bool? LineMetrics { get; set; }

    /// <summary>
    /// Clusters point features. Defaults to false.
    /// </summary>
    public bool? Cluster { get; set; }

    /// <summary>
    /// Tiles the data for the next render update instead of in the background.
    /// Defaults to false.
    /// </summary>
    public bool? SynchronousTiling { get; set; }
    public static GeojsonSourceOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyGeojsonSourceOptions(
                NativeMethods.mln_geojson_source_options_default()
            );
        }
    }
}
