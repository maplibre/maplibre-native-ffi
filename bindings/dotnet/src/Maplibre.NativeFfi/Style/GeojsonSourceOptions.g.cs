// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public sealed record GeojsonSourceOptions
{
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public double? Tolerance { get; set; }
    public double? ClusterMaxZoom { get; set; }
    public byte[]? ClusterProperties
    {
        get => ClusterPropertiesStorage?.ToArray();
        set => ClusterPropertiesStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? ClusterPropertiesStorage { get; set; }
    public uint? TileSize { get; set; }
    public uint? Buffer { get; set; }
    public uint? ClusterRadius { get; set; }
    public uint? ClusterMinPoints { get; set; }
    public bool? LineMetrics { get; set; }
    public bool? Cluster { get; set; }
    public bool? SynchronousTiling { get; set; }
    public static GeojsonSourceOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_geojson_source_options_default");
            return CopyGeojsonSourceOptions(NativeMethods.mln_geojson_source_options_default());
        }
    }
}
