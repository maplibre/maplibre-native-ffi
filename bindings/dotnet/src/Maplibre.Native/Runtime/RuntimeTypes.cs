using Maplibre.Native.Geo;
using Maplibre.Native.Map;
using Maplibre.Native.Offline;
using Maplibre.Native.Render;
using Maplibre.Native.Resource;

namespace Maplibre.Native.Runtime;

public enum AmbientCacheOperation : uint
{
    ResetDatabase = 1,
    PackDatabase = 2,
    Invalidate = 3,
    Clear = 4,
}

public enum OfflineOperationKind : uint
{
    AmbientCache = 1,
    RegionCreate = 2,
    RegionGet = 3,
    RegionsList = 4,
    RegionsMergeDatabase = 5,
    RegionUpdateMetadata = 6,
    RegionGetStatus = 7,
    RegionSetObserved = 8,
    RegionSetDownloadState = 9,
    RegionInvalidate = 10,
    RegionDelete = 11,
}

public enum OfflineOperationResultKind : uint
{
    None = 0,
    Region = 1,
    OptionalRegion = 2,
    RegionList = 3,
    RegionStatus = 4,
}

public enum RuntimeEventSourceType : uint
{
    Runtime = 0,
    Map = 1,
}

public enum RuntimeEventType : uint
{
    MapCameraWillChange = 1,
    MapCameraIsChanging = 2,
    MapCameraDidChange = 3,
    MapStyleLoaded = 4,
    MapLoadingStarted = 5,
    MapLoadingFinished = 6,
    MapLoadingFailed = 7,
    MapIdle = 8,
    MapRenderUpdateAvailable = 9,
    MapRenderError = 10,
    MapStillImageFinished = 11,
    MapStillImageFailed = 12,
    MapRenderFrameStarted = 13,
    MapRenderFrameFinished = 14,
    MapRenderMapStarted = 15,
    MapRenderMapFinished = 16,
    MapStyleImageMissing = 17,
    MapTileAction = 18,
    OfflineRegionStatusChanged = 19,
    OfflineRegionResponseError = 20,
    OfflineRegionTileCountLimitExceeded = 21,
    OfflineOperationCompleted = 22,
}

public sealed record RuntimeEvent(
    RuntimeEventType Type,
    uint RawType,
    RuntimeEventSourceType SourceType,
    uint RawSourceType,
    RuntimeHandle? RuntimeSource,
    MapHandle? MapSource,
    int Code,
    uint RawPayloadType,
    RuntimeEventPayload Payload,
    string Message
);

public abstract record RuntimeEventPayload
{
    private RuntimeEventPayload() { }

    public sealed record None : RuntimeEventPayload
    {
        public static None Instance { get; } = new();

        private None() { }
    }

    public sealed record RenderFrame(
        RenderMode Mode,
        uint RawMode,
        bool NeedsRepaint,
        bool PlacementChanged,
        RenderingStats Stats
    ) : RuntimeEventPayload;

    public sealed record RenderMap(RenderMode Mode, uint RawMode) : RuntimeEventPayload;

    public sealed record StyleImageMissing(string ImageId) : RuntimeEventPayload;

    public sealed record TileAction(
        TileOperation Operation,
        uint RawOperation,
        TileId TileId,
        string SourceId
    ) : RuntimeEventPayload;

    public sealed record OfflineRegionStatusChanged(
        long RegionId,
        global::Maplibre.Native.Offline.OfflineRegionStatus Status
    ) : RuntimeEventPayload;

    public sealed record OfflineRegionResponseError(
        long RegionId,
        ResourceErrorReason Reason,
        uint RawReason
    ) : RuntimeEventPayload;

    public sealed record OfflineRegionTileCountLimit(long RegionId, ulong Limit)
        : RuntimeEventPayload;

    public sealed record OfflineOperationCompleted(
        ulong OperationId,
        OfflineOperationKind OperationKind,
        uint RawOperationKind,
        OfflineOperationResultKind ResultKind,
        uint RawResultKind,
        int ResultStatus,
        bool Found
    ) : RuntimeEventPayload;

    public sealed record Unknown : RuntimeEventPayload
    {
        private readonly byte[] payloadBytes;

        public Unknown(uint RawPayloadType, byte[] PayloadBytes)
        {
            ArgumentNullException.ThrowIfNull(PayloadBytes);
            this.RawPayloadType = RawPayloadType;
            payloadBytes = (byte[])PayloadBytes.Clone();
        }

        public uint RawPayloadType { get; }
        public byte[] PayloadBytes => (byte[])payloadBytes.Clone();
    }
}
