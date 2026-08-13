using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Resource;

namespace Maplibre.NativeFfi.Runtime;

public enum AmbientCacheOperation : uint
{
    ResetDatabase = 1,
    PackDatabase = 2,
    Invalidate = 3,
    Clear = 4,
}

/// <summary>The terminal state copied from a completed operation.</summary>
public sealed record OperationCompletion(MaplibreStatus Status, int RawStatus, string Diagnostic);

public enum CommandDisposition : uint
{
    Committed = 0,
    Superseded = 1,
    Failed = 2,
    Cancelled = 3,
}

public enum NotificationEndpointKind : uint
{
    Unknown = 0,
    RuntimeEvents = 1,
    Operation = 2,
    AdapterResourceRequests = 3,
    AdapterLogRecords = 4,
    RenderFrames = 5,
    DriverWork = 6,
}

/// <summary>One copied endpoint from a notification-source ready batch.</summary>
public sealed record ReadyEndpoint(NotificationEndpointKind Kind, uint RawKind, ulong Id);

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
    MapCameraTransitionFinished = 23,
    CommandFinished = 24,
}

/// <summary>Event types a map or a runtime queues.</summary>
/// <remarks>
/// Each bit is <c>1</c> shifted left by the <see cref="RuntimeEventType" /> value it selects, so a
/// mask a host computes from a decoded event type matches these constants.
/// <para>
/// <see cref="MapHandle.SetEventMask" /> reads the bits in <see cref="AllMapEvents" /> and
/// <see cref="RuntimeHandle.SetEventMask" /> reads the bits in <see cref="AllRuntimeEvents" />, so
/// both accept <see cref="All" /> and a host reads a mask, changes one bit, and writes it back.
/// </para>
/// </remarks>
[Flags]
public enum RuntimeEventMask : ulong
{
    /// <summary>Selects no event type.</summary>
    None = 0,
    MapCameraWillChange = 1UL << (int)RuntimeEventType.MapCameraWillChange,
    MapCameraIsChanging = 1UL << (int)RuntimeEventType.MapCameraIsChanging,
    MapCameraDidChange = 1UL << (int)RuntimeEventType.MapCameraDidChange,
    MapStyleLoaded = 1UL << (int)RuntimeEventType.MapStyleLoaded,
    MapLoadingStarted = 1UL << (int)RuntimeEventType.MapLoadingStarted,
    MapLoadingFinished = 1UL << (int)RuntimeEventType.MapLoadingFinished,
    MapLoadingFailed = 1UL << (int)RuntimeEventType.MapLoadingFailed,
    MapIdle = 1UL << (int)RuntimeEventType.MapIdle,
    MapRenderUpdateAvailable = 1UL << (int)RuntimeEventType.MapRenderUpdateAvailable,
    MapRenderError = 1UL << (int)RuntimeEventType.MapRenderError,
    MapStillImageFinished = 1UL << (int)RuntimeEventType.MapStillImageFinished,
    MapStillImageFailed = 1UL << (int)RuntimeEventType.MapStillImageFailed,
    MapRenderFrameStarted = 1UL << (int)RuntimeEventType.MapRenderFrameStarted,
    MapRenderFrameFinished = 1UL << (int)RuntimeEventType.MapRenderFrameFinished,
    MapRenderMapStarted = 1UL << (int)RuntimeEventType.MapRenderMapStarted,
    MapRenderMapFinished = 1UL << (int)RuntimeEventType.MapRenderMapFinished,
    MapStyleImageMissing = 1UL << (int)RuntimeEventType.MapStyleImageMissing,
    MapTileAction = 1UL << (int)RuntimeEventType.MapTileAction,
    MapCameraTransitionFinished = 1UL << (int)RuntimeEventType.MapCameraTransitionFinished,
    CommandFinished = 1UL << (int)RuntimeEventType.CommandFinished,
    OfflineRegionStatusChanged = 1UL << (int)RuntimeEventType.OfflineRegionStatusChanged,
    OfflineRegionResponseError = 1UL << (int)RuntimeEventType.OfflineRegionResponseError,
    OfflineRegionTileCountLimitExceeded =
        1UL << (int)RuntimeEventType.OfflineRegionTileCountLimitExceeded,

    /// <summary>Selects every map-originated event type this binding declares.</summary>
    AllMapEvents =
        MapCameraWillChange
        | MapCameraIsChanging
        | MapCameraDidChange
        | MapStyleLoaded
        | MapLoadingStarted
        | MapLoadingFinished
        | MapLoadingFailed
        | MapIdle
        | MapRenderUpdateAvailable
        | MapRenderError
        | MapStillImageFinished
        | MapStillImageFailed
        | MapRenderFrameStarted
        | MapRenderFrameFinished
        | MapRenderMapStarted
        | MapRenderMapFinished
        | MapStyleImageMissing
        | MapTileAction
        | MapCameraTransitionFinished
        | CommandFinished,

    /// <summary>Selects every runtime-originated event type this binding declares.</summary>
    AllRuntimeEvents =
        OfflineRegionStatusChanged
        | OfflineRegionResponseError
        | OfflineRegionTileCountLimitExceeded
        | CommandFinished,

    /// <summary>Selects every event type this binding declares.</summary>
    All = AllMapEvents | AllRuntimeEvents,
}

/// <summary>One drained batch of copied runtime events.</summary>
/// <remarks>
/// Every value in the batch is a copy, so it stays readable after the next drain and after the map
/// that produced it is closed.
/// </remarks>
/// <param name="Events">The copied events in queue order.</param>
/// <param name="RemainingCount">
/// Events still queued for the runtime after this batch. A nonzero value means another drain
/// reports more events.
/// </param>
public sealed record RuntimeEventBatch(IReadOnlyList<RuntimeEvent> Events, ulong RemainingCount)
{
    private readonly IReadOnlyList<RuntimeEvent> events = ValueEquality.Snapshot(Events);

    public IReadOnlyList<RuntimeEvent> Events
    {
        get => events;
        init => events = ValueEquality.Snapshot(value);
    }

    public bool Equals(RuntimeEventBatch? other) =>
        other is not null
        && ValueEquality.SequenceEquals(events, other.events)
        && RemainingCount == other.RemainingCount;

    public override int GetHashCode() =>
        HashCode.Combine(ValueEquality.SequenceHashCode(events), RemainingCount);
}

/// <summary>One copied runtime event from <see cref="RuntimeHandle.DrainEvents()" />.</summary>
/// <param name="Type">The event kind, when this binding knows the raw value.</param>
/// <param name="RawType">The raw native event kind.</param>
/// <param name="SourceType">The source kind, when this binding knows the raw value.</param>
/// <param name="RawSourceType">The raw native source kind.</param>
/// <param name="RawSource">
/// The raw native source identity, which names one object for the life of the process. Every event
/// carries it, including a source kind this binding does not name and a map source that resolves to
/// no live <see cref="MapHandle" />, so a host still correlates or forwards an event whose source
/// this binding cannot resolve. It is an identity value only: it compares and hashes, and it grants
/// no operations on the object it names.
/// </param>
/// <param name="RuntimeSource">The runtime that produced a runtime-sourced event.</param>
/// <param name="MapSource">
/// The map that raised this event, resolved from the runtime's weak map registry.
/// Hold a strong reference to the <see cref="MapHandle" /> for as long as you want to
/// attribute its events: a collected map leaves this <see langword="null" /> even
/// though <see cref="SourceType" /> still reports a map source.
/// </param>
/// <param name="Code">
/// Secondary event detail whose meaning <paramref name="Type" /> selects:
/// <list type="bullet">
/// <item>
/// <description>
/// <see cref="RuntimeEventType.MapCameraWillChange" /> and
/// <see cref="RuntimeEventType.MapCameraDidChange" />: a
/// <see cref="Maplibre.NativeFfi.Camera.CameraChangeMode" /> value.
/// </description>
/// </item>
/// <item>
/// <description>
/// <see cref="RuntimeEventType.MapLoadingFailed" />: the ordinal of MapLibre Native's internal map
/// load error kind, which this binding does not name as an enum. Read
/// <paramref name="Message" /> for the failure text.
/// </description>
/// </item>
/// <item><description>Every other event kind: 0.</description></item>
/// </list>
/// </param>
/// <param name="RawPayloadType">The raw native payload kind.</param>
/// <param name="Payload">The copied typed payload selected by the payload kind.</param>
/// <param name="Message">
/// The copied event message, empty when the event carries none. It carries the image ID for
/// <see cref="RuntimeEventType.MapStyleImageMissing" />, the source ID for
/// <see cref="RuntimeEventType.MapTileAction" />, and native failure text for the loading and
/// render error kinds.
/// </param>
public sealed record RuntimeEvent(
    RuntimeEventType Type,
    uint RawType,
    RuntimeEventSourceType SourceType,
    uint RawSourceType,
    ulong RawSource,
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

    /// <summary>Payload for a map tile action event.</summary>
    /// <remarks>The event message carries the source ID.</remarks>
    public sealed record TileAction(TileOperation Operation, uint RawOperation, TileId TileId)
        : RuntimeEventPayload;

    public sealed record OfflineRegionStatusChanged(
        long RegionId,
        global::Maplibre.NativeFfi.Offline.OfflineRegionStatus Status
    ) : RuntimeEventPayload;

    public sealed record OfflineRegionResponseError(
        long RegionId,
        ResourceErrorReason Reason,
        uint RawReason
    ) : RuntimeEventPayload;

    public sealed record OfflineRegionTileCountLimit(long RegionId, ulong Limit)
        : RuntimeEventPayload;

    /// <summary>Payload for a map camera transition-finished event.</summary>
    /// <param name="TransitionId">
    /// The transition ID the caller set on the <c>AnimationOptions</c> that started this
    /// transition. See <c>AnimationOptions.TransitionId</c> for the terminal outcomes this event
    /// covers.
    /// </param>
    public sealed record CameraTransitionFinished(ulong TransitionId) : RuntimeEventPayload;

    /// <summary>Terminal outcome of an accepted ordered command.</summary>
    public sealed record CommandFinished(
        ulong CommandId,
        CommandDisposition Disposition,
        uint RawDisposition,
        ulong Generation
    ) : RuntimeEventPayload;

    /// <summary>Payload for a payload kind this binding does not declare.</summary>
    /// <remarks>
    /// <see cref="PayloadBytes" /> holds the event record's fixed payload window, copied unchanged,
    /// so a host forwards a payload kind a later library version adds.
    /// </remarks>
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
