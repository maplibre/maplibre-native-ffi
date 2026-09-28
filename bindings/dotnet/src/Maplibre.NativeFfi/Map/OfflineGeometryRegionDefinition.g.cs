// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

public readonly record struct OfflineGeometryRegionDefinition
{
    public OfflineGeometryRegionDefinition(
        string StyleUrl,
        byte[] Geometry,
        double MinZoom,
        double MaxZoom,
        float PixelRatio,
        bool IncludeIdeographs
    )
        : this(StyleUrl, Geometry, MinZoom, MaxZoom, PixelRatio, IncludeIdeographs, false) { }

    internal OfflineGeometryRegionDefinition(
        string StyleUrl,
        byte[] Geometry,
        double MinZoom,
        double MaxZoom,
        float PixelRatio,
        bool IncludeIdeographs,
        bool adopt
    )
    {
        this.StyleUrl = StyleUrl;
        this.storageGeometry = adopt ? Geometry : Geometry?.ToArray() ?? [];
        this.MinZoom = MinZoom;
        this.MaxZoom = MaxZoom;
        this.PixelRatio = PixelRatio;
        this.IncludeIdeographs = IncludeIdeographs;
    }

    public string StyleUrl { get; init; }
    private readonly byte[]? storageGeometry;
    public byte[] Geometry
    {
        get => storageGeometry?.ToArray() ?? [];
        init => storageGeometry = value?.ToArray() ?? [];
    }
    internal byte[] GeometryStorage
    {
        get => storageGeometry ?? [];
        init => storageGeometry = value;
    }
    public double MinZoom { get; init; }
    public double MaxZoom { get; init; }
    public float PixelRatio { get; init; }
    public bool IncludeIdeographs { get; init; }

    public bool Equals(OfflineGeometryRegionDefinition other) =>
        EqualityComparer<string>.Default.Equals(StyleUrl, other.StyleUrl)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            GeometryStorage,
            other.GeometryStorage
        )
        && EqualityComparer<double>.Default.Equals(MinZoom, other.MinZoom)
        && EqualityComparer<double>.Default.Equals(MaxZoom, other.MaxZoom)
        && EqualityComparer<float>.Default.Equals(PixelRatio, other.PixelRatio)
        && EqualityComparer<bool>.Default.Equals(IncludeIdeographs, other.IncludeIdeographs);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(StyleUrl);
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(GeometryStorage)
        );
        hash.Add(MinZoom);
        hash.Add(MaxZoom);
        hash.Add(PixelRatio);
        hash.Add(IncludeIdeographs);
        return hash.ToHashCode();
    }
}
