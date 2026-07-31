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
    SetMaximumAmbientCacheSize = 12,
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
    MapCameraTransitionFinished = 23,
}

/// <summary>One copied runtime event returned by <c>RuntimeHandle.PollEvent</c>.</summary>
/// <param name="Type">The event kind, when this binding knows the raw value.</param>
/// <param name="RawType">The raw native event kind.</param>
/// <param name="SourceType">The source kind, when this binding knows the raw value.</param>
/// <param name="RawSourceType">The raw native source kind.</param>
/// <param name="RuntimeSource">The runtime that produced a runtime-sourced event.</param>
/// <param name="MapSource">
/// The map that raised this event, resolved from the runtime's weak map registry.
/// Hold your own strong reference to a <see cref="MapHandle" /> for as long as you
/// want to attribute its events: once the only remaining reference is the runtime's
/// weak one, the map can be collected and this member is <see langword="null" />
/// even though <see cref="SourceType" /> still reports a map source.
/// </param>
/// <param name="Code">
/// Secondary event detail whose meaning <paramref name="Type" /> selects:
/// <list type="bullet">
/// <item>
/// <description>
/// <see cref="RuntimeEventType.MapCameraWillChange" /> and
/// <see cref="RuntimeEventType.MapCameraDidChange" />: a
/// <see cref="Maplibre.Native.Camera.CameraChangeMode" /> value.
/// </description>
/// </item>
/// <item>
/// <description>
/// <see cref="RuntimeEventType.MapLoadingFailed" />: the ordinal of MapLibre Native's internal map
/// load error kind, which this binding does not name as an enum. Read
/// <paramref name="Message" /> for the failure text.
/// </description>
/// </item>
/// <item>
/// <description>
/// <see cref="RuntimeEventType.OfflineOperationCompleted" />: the operation result as a
/// <c>MaplibreStatus</c> value, the same value the payload reports in its result status.
/// </description>
/// </item>
/// <item><description>Every other event kind: 0.</description></item>
/// </list>
/// </param>
/// <param name="RawPayloadType">The raw native payload kind.</param>
/// <param name="Payload">The copied typed payload selected by the payload kind.</param>
/// <param name="Message">The copied event message, empty when the event carries none.</param>
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

    /// <summary>Payload for a map camera transition-finished event.</summary>
    /// <param name="TransitionId">
    /// The transition ID the caller set on the <c>AnimationOptions</c> that started this
    /// transition. See <c>AnimationOptions.TransitionId</c> for the terminal outcomes this event
    /// covers.
    /// </param>
    public sealed record CameraTransitionFinished(ulong TransitionId) : RuntimeEventPayload;

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

        public bool Equals(Unknown? other) =>
            other is not null
            && RawPayloadType == other.RawPayloadType
            && payloadBytes.AsSpan().SequenceEqual(other.payloadBytes);

        public override int GetHashCode()
        {
            var hash = new HashCode();
            hash.Add(RawPayloadType);
            hash.AddBytes(payloadBytes);
            return hash.ToHashCode();
        }
    }
}
