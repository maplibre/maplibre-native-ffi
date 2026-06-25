using Maplibre.Native.Geo;

namespace Maplibre.Native.Offline;

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

    public sealed record GeometryRegion(
        string StyleUrl,
        Geometry Geometry,
        double MinimumZoom,
        double MaximumZoom,
        float PixelRatio,
        bool IncludeIdeographs
    ) : OfflineRegionDefinition;
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
