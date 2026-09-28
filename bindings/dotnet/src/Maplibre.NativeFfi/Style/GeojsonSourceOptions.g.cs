// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Style;

public sealed record GeojsonSourceOptions
{
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public double? Tolerance { get; set; }
    public double? ClusterMaxZoom { get; set; }
    private byte[]? storageClusterProperties;
    public byte[]? ClusterProperties
    {
        get => storageClusterProperties?.ToArray();
        set => storageClusterProperties = value?.ToArray();
    }
    internal byte[]? ClusterPropertiesStorage
    {
        get => storageClusterProperties;
        init => storageClusterProperties = value;
    }
    public uint? TileSize { get; set; }
    public uint? Buffer { get; set; }
    public uint? ClusterRadius { get; set; }
    public uint? ClusterMinPoints { get; set; }
    public bool? LineMetrics { get; set; }
    public bool? Cluster { get; set; }
    public bool? SynchronousTiling { get; set; }

    public bool Equals(GeojsonSourceOptions? other) =>
        other is not null
        && EqualityComparer<double?>.Default.Equals(MinZoom, other.MinZoom)
        && EqualityComparer<double?>.Default.Equals(MaxZoom, other.MaxZoom)
        && EqualityComparer<double?>.Default.Equals(Tolerance, other.Tolerance)
        && EqualityComparer<double?>.Default.Equals(ClusterMaxZoom, other.ClusterMaxZoom)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            ClusterPropertiesStorage,
            other.ClusterPropertiesStorage
        )
        && EqualityComparer<uint?>.Default.Equals(TileSize, other.TileSize)
        && EqualityComparer<uint?>.Default.Equals(Buffer, other.Buffer)
        && EqualityComparer<uint?>.Default.Equals(ClusterRadius, other.ClusterRadius)
        && EqualityComparer<uint?>.Default.Equals(ClusterMinPoints, other.ClusterMinPoints)
        && EqualityComparer<bool?>.Default.Equals(LineMetrics, other.LineMetrics)
        && EqualityComparer<bool?>.Default.Equals(Cluster, other.Cluster)
        && EqualityComparer<bool?>.Default.Equals(SynchronousTiling, other.SynchronousTiling);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(MinZoom);
        hash.Add(MaxZoom);
        hash.Add(Tolerance);
        hash.Add(ClusterMaxZoom);
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(
                ClusterPropertiesStorage
            )
        );
        hash.Add(TileSize);
        hash.Add(Buffer);
        hash.Add(ClusterRadius);
        hash.Add(ClusterMinPoints);
        hash.Add(LineMetrics);
        hash.Add(Cluster);
        hash.Add(SynchronousTiling);
        return hash.ToHashCode();
    }

    public static GeojsonSourceOptions Default
    {
        get
        {
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_geojson_source_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyGeojsonSourceOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_geojson_source_options_default()
            );
        }
    }
}
