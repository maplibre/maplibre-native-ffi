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
    public double? MinZoom { get; init; }

    /// <summary>
    /// Maximum tiling zoom, an integer within [0, 24]. Defaults to 18.
    /// </summary>
    public double? MaxZoom { get; init; }

    /// <summary>
    /// Douglas-Peucker simplification tolerance. Defaults to 0.375.
    /// </summary>
    public double? Tolerance { get; init; }

    /// <summary>
    /// Highest zoom that clusters points. Defaults to 17.
    /// </summary>
    public double? ClusterMaxZoom { get; init; }

    /// <summary>
    /// Cluster aggregation expressions keyed by property name, as a JSON object
    /// whose members follow the MapLibre Style Spec clusterProperties form, or
    /// empty for none. The UTF-8 bytes are borrowed for the call.
    /// </summary>
    public ImmutableArray<byte> ClusterProperties
    {
        get => ClusterPropertiesStorage.AsImmutable();
        init => ClusterPropertiesStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> ClusterPropertiesStorage { get; init; }

    /// <summary>
    /// Tile buffer in pixels within [0, 512]. Defaults to 128.
    /// </summary>
    public uint? Buffer { get; init; }

    /// <summary>
    /// Cluster radius in pixels within [0, 4095]. Defaults to 50.
    /// </summary>
    public uint? ClusterRadius { get; init; }

    /// <summary>
    /// Points required to form a cluster. Defaults to 2.
    /// </summary>
    public uint? ClusterMinPoints { get; init; }

    /// <summary>
    /// Adds line distance metrics to line features. Defaults to false.
    /// </summary>
    public bool? LineMetrics { get; init; }

    /// <summary>
    /// Clusters point features. Defaults to false.
    /// </summary>
    public bool? Cluster { get; init; }

    /// <summary>
    /// Tiles the data for the next render update instead of in the background.
    /// Defaults to false.
    /// </summary>
    public bool? SynchronousTiling { get; init; }
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
