using Maplibre.NativeFfi.Geo;

namespace Maplibre.NativeFfi.Offline;

public enum OfflineRegionDownloadState : uint
{
    Inactive = 0,
    Active = 1,
}

public abstract record OfflineRegionDefinition
{
    private OfflineRegionDefinition() { }

    public sealed record TilePyramid(
        string StyleUrl,
        LatLngBounds Bounds,
        double MinimumZoom,
        double MaximumZoom,
        float PixelRatio,
        bool IncludeIdeographs
    ) : OfflineRegionDefinition;

    public sealed record GeometryRegion : OfflineRegionDefinition
    {
        private readonly byte[] geometry;

        public GeometryRegion(
            string StyleUrl,
            byte[] Geometry,
            double MinimumZoom,
            double MaximumZoom,
            float PixelRatio,
            bool IncludeIdeographs
        )
        {
            ArgumentNullException.ThrowIfNull(Geometry);
            this.StyleUrl = StyleUrl;
            geometry = (byte[])Geometry.Clone();
            this.MinimumZoom = MinimumZoom;
            this.MaximumZoom = MaximumZoom;
            this.PixelRatio = PixelRatio;
            this.IncludeIdeographs = IncludeIdeographs;
        }

        public string StyleUrl { get; }
        public byte[] Geometry => (byte[])geometry.Clone();
        public double MinimumZoom { get; }
        public double MaximumZoom { get; }
        public float PixelRatio { get; }
        public bool IncludeIdeographs { get; }

        public bool Equals(GeometryRegion? other) =>
            other is not null
            && StyleUrl == other.StyleUrl
            && geometry.AsSpan().SequenceEqual(other.geometry)
            && MinimumZoom == other.MinimumZoom
            && MaximumZoom == other.MaximumZoom
            && PixelRatio == other.PixelRatio
            && IncludeIdeographs == other.IncludeIdeographs;

        public override int GetHashCode()
        {
            var hash = new HashCode();
            hash.Add(StyleUrl);
            hash.AddBytes(geometry);
            hash.Add(MinimumZoom);
            hash.Add(MaximumZoom);
            hash.Add(PixelRatio);
            hash.Add(IncludeIdeographs);
            return hash.ToHashCode();
        }
    }
}

public sealed record OfflineRegionInfo
{
    private readonly byte[] metadata;

    public OfflineRegionInfo(long Id, OfflineRegionDefinition Definition, byte[] Metadata)
    {
        ArgumentNullException.ThrowIfNull(Metadata);
        this.Id = Id;
        this.Definition = Definition;
        metadata = (byte[])Metadata.Clone();
    }

    public long Id { get; }
    public OfflineRegionDefinition Definition { get; }
    public byte[] Metadata => (byte[])metadata.Clone();

    public bool Equals(OfflineRegionInfo? other) =>
        other is not null
        && Id == other.Id
        && Equals(Definition, other.Definition)
        && metadata.AsSpan().SequenceEqual(other.metadata);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Id);
        hash.Add(Definition);
        hash.AddBytes(metadata);
        return hash.ToHashCode();
    }
}

public sealed record OfflineRegionStatus(
    OfflineRegionDownloadState DownloadState,
    ulong CompletedResourceCount,
    ulong CompletedResourceSize,
    ulong CompletedTileCount,
    ulong RequiredTileCount,
    ulong CompletedTileSize,
    ulong RequiredResourceCount,
    bool RequiredResourceCountIsPrecise,
    bool Complete
);
