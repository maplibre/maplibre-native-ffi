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

public sealed record OfflineRegionInfo(
    long Id,
    OfflineRegionDefinition Definition,
    byte[] Metadata
);

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
