using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Resource;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Internal.Struct;

internal static unsafe class RuntimeStructs
{
    /// <summary>
    /// Offset of the payload union inside one event record, which every C API version keeps, so it
    /// also bounds the opaque window of a payload kind this binding does not declare.
    /// </summary>
    private static readonly uint PayloadOffset = (uint)
        Marshal.OffsetOf<mln_runtime_event>(nameof(mln_runtime_event.payload));

    internal static List<RuntimeEvent> ReadBatch(in mln_runtime_event_batch_view batch) =>
        ReadBatch(batch, null, static _ => null);

    /// <summary>Copies every event from an owned batch view, in queue order.</summary>
    /// <remarks>
    /// Events are indexed by the stride the batch reports rather than by this binding's own record
    /// size, so a later C API version that widens the payload union stays readable.
    /// </remarks>
    internal static List<RuntimeEvent> ReadBatch(
        in mln_runtime_event_batch_view batch,
        RuntimeHandle? runtimeSource,
        Func<ulong, MapHandle?> mapSource
    )
    {
        var count = checked((int)batch.event_count);
        var events = new List<RuntimeEvent>(count);
        var records = (byte*)batch.events;
        for (var index = 0; index < count; index++)
        {
            events.Add(
                ReadEvent(
                    (mln_runtime_event*)(records + (nuint)index * batch.event_size),
                    batch.event_size,
                    batch.messages,
                    runtimeSource,
                    mapSource
                )
            );
        }
        return events;
    }

    private static RuntimeEvent ReadEvent(
        mln_runtime_event* record,
        uint eventSize,
        sbyte* messages,
        RuntimeHandle? runtimeSource,
        Func<ulong, MapHandle?> mapSource
    )
    {
        var sourceType = (RuntimeEventSourceType)record->source_type;
        return new RuntimeEvent(
            (RuntimeEventType)record->type,
            record->type,
            sourceType,
            record->source_type,
            record->source,
            sourceType == RuntimeEventSourceType.Runtime ? runtimeSource : null,
            sourceType == RuntimeEventSourceType.Map ? mapSource(record->source) : null,
            record->code,
            record->payload_type,
            ReadPayload(record, eventSize),
            CopyUtf8(messages + record->message_offset, record->message_size)
        );
    }

    private static RuntimeEventPayload ReadPayload(mln_runtime_event* record, uint eventSize) =>
        (mln_runtime_event_payload_type)record->payload_type switch
        {
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_NONE => RuntimeEventPayload
                .None
                .Instance,
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME =>
                ReadRenderFrame(record->payload.render_frame),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP => ReadRenderMap(
                record->payload.render_map
            ),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION => ReadTileAction(
                record->payload.tile_action
            ),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS =>
                ReadOfflineRegionStatus(record->payload.offline_region_status),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR =>
                ReadOfflineRegionResponseError(record->payload.offline_region_response_error),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT =>
                ReadOfflineRegionTileCountLimit(record->payload.offline_region_tile_count_limit),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED =>
                ReadCameraTransitionFinished(record->payload.camera_transition_finished),
            _ => new RuntimeEventPayload.Unknown(
                record->payload_type,
                CopyBytes((byte*)record + PayloadOffset, eventSize - PayloadOffset)
            ),
        };

    private static RuntimeEventPayload.RenderFrame ReadRenderFrame(
        in mln_runtime_event_render_frame payload
    ) =>
        new(
            (RenderMode)payload.mode,
            payload.mode,
            payload.needs_repaint != 0,
            payload.placement_changed != 0,
            RenderingStats(payload.stats)
        );

    private static RuntimeEventPayload.RenderMap ReadRenderMap(
        in mln_runtime_event_render_map payload
    ) => new((RenderMode)payload.mode, payload.mode);

    private static RuntimeEventPayload.TileAction ReadTileAction(
        in mln_runtime_event_tile_action payload
    ) => new((TileOperation)payload.operation, payload.operation, TileId(payload.tile_id));

    private static RuntimeEventPayload.OfflineRegionStatusChanged ReadOfflineRegionStatus(
        in mln_runtime_event_offline_region_status payload
    ) => new(payload.region_id, OfflineRegionStatus(payload.status));

    private static RuntimeEventPayload.OfflineRegionResponseError ReadOfflineRegionResponseError(
        in mln_runtime_event_offline_region_response_error payload
    ) => new(payload.region_id, (ResourceErrorReason)payload.reason, payload.reason);

    private static RuntimeEventPayload.OfflineRegionTileCountLimit ReadOfflineRegionTileCountLimit(
        in mln_runtime_event_offline_region_tile_count_limit payload
    ) => new(payload.region_id, payload.limit);

    private static RuntimeEventPayload.CameraTransitionFinished ReadCameraTransitionFinished(
        in mln_runtime_event_camera_transition_finished payload
    ) => new(payload.transition_id);

    private static RenderingStats RenderingStats(mln_rendering_stats value) =>
        new(
            value.encoding_time,
            value.rendering_time,
            value.frame_count,
            value.draw_call_count,
            value.total_draw_call_count
        );

    private static TileId TileId(mln_tile_id value) =>
        new(
            value.overscaled_z,
            value.wrap,
            value.canonical_z,
            value.canonical_x,
            value.canonical_y
        );

    private static OfflineRegionStatus OfflineRegionStatus(mln_offline_region_status value) =>
        new(
            (OfflineRegionDownloadState)value.download_state,
            value.completed_resource_count,
            value.completed_resource_size,
            value.completed_tile_count,
            value.required_tile_count,
            value.completed_tile_size,
            value.required_resource_count,
            value.required_resource_count_is_precise != 0,
            value.complete != 0
        );

    internal static string CopyUtf8(sbyte* pointer, nuint byteLength)
    {
        if (pointer is null || byteLength == 0)
        {
            return string.Empty;
        }

        return Marshal.PtrToStringUTF8((nint)pointer, checked((int)byteLength)) ?? string.Empty;
    }

    internal static string CopyUtf8(void* pointer, nuint byteLength) =>
        CopyUtf8((sbyte*)pointer, byteLength);

    private static byte[] CopyBytes(byte* pointer, nuint byteLength)
    {
        if (pointer is null || byteLength == 0)
        {
            return [];
        }

        var bytes = new byte[checked((int)byteLength)];
        Marshal.Copy((nint)pointer, bytes, 0, bytes.Length);
        return bytes;
    }
}
