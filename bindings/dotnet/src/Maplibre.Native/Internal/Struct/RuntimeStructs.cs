using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Maplibre.Native.Geo;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Map;
using Maplibre.Native.Offline;
using Maplibre.Native.Render;
using Maplibre.Native.Resource;
using Maplibre.Native.Runtime;

namespace Maplibre.Native.Internal.Struct;

internal static unsafe class RuntimeStructs
{
    internal static RuntimeEvent ReadEvent(in mln_runtime_event raw) =>
        ReadEvent(raw, null, _ => null);

    internal static RuntimeEvent ReadEvent(
        in mln_runtime_event raw,
        RuntimeHandle? runtimeSource,
        Func<nint, MapHandle?> mapSource
    )
    {
        var sourceType = (RuntimeEventSourceType)raw.source_type;
        return new RuntimeEvent(
            (RuntimeEventType)raw.type,
            raw.type,
            sourceType,
            raw.source_type,
            sourceType == RuntimeEventSourceType.Runtime ? runtimeSource : null,
            sourceType == RuntimeEventSourceType.Map ? mapSource((nint)raw.source) : null,
            raw.code,
            raw.payload_type,
            ReadPayload(raw.payload_type, raw.payload, raw.payload_size),
            CopyUtf8(raw.message, raw.message_size)
        );
    }

    private static RuntimeEventPayload ReadPayload(
        uint payloadType,
        void* payload,
        nuint payloadSize
    )
    {
        if (
            payload is null
            || payloadType == (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_NONE
        )
        {
            return RuntimeEventPayload.None.Instance;
        }

        return (mln_runtime_event_payload_type)payloadType switch
        {
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME
                when HasPayload<mln_runtime_event_render_frame>(payloadSize) => ReadRenderFrame(
                (mln_runtime_event_render_frame*)payload
            ),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
                when HasPayload<mln_runtime_event_render_map>(payloadSize) => ReadRenderMap(
                (mln_runtime_event_render_map*)payload
            ),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING
                when HasPayload<mln_runtime_event_style_image_missing>(payloadSize) =>
                ReadStyleImageMissing((mln_runtime_event_style_image_missing*)payload),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
                when HasPayload<mln_runtime_event_tile_action>(payloadSize) => ReadTileAction(
                (mln_runtime_event_tile_action*)payload
            ),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS
                when HasPayload<mln_runtime_event_offline_region_status>(payloadSize) =>
                ReadOfflineRegionStatus((mln_runtime_event_offline_region_status*)payload),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR
                when HasPayload<mln_runtime_event_offline_region_response_error>(payloadSize) =>
                ReadOfflineRegionResponseError(
                    (mln_runtime_event_offline_region_response_error*)payload
                ),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT
                when HasPayload<mln_runtime_event_offline_region_tile_count_limit>(payloadSize) =>
                ReadOfflineRegionTileCountLimit(
                    (mln_runtime_event_offline_region_tile_count_limit*)payload
                ),
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
                when HasPayload<mln_runtime_event_offline_operation_completed>(payloadSize) =>
                ReadOfflineOperationCompleted(
                    (mln_runtime_event_offline_operation_completed*)payload
                ),
            _ => new RuntimeEventPayload.Unknown(
                payloadType,
                CopyBytes((byte*)payload, payloadSize)
            ),
        };
    }

    private static bool HasPayload<T>(nuint payloadSize)
        where T : unmanaged => payloadSize >= (nuint)sizeof(T);

    private static RuntimeEventPayload.RenderFrame ReadRenderFrame(
        mln_runtime_event_render_frame* payload
    ) =>
        new(
            (RenderMode)payload->mode,
            payload->mode,
            payload->needs_repaint != 0,
            payload->placement_changed != 0,
            RenderingStats(payload->stats)
        );

    private static RuntimeEventPayload.RenderMap ReadRenderMap(
        mln_runtime_event_render_map* payload
    ) => new((RenderMode)payload->mode, payload->mode);

    private static RuntimeEventPayload.StyleImageMissing ReadStyleImageMissing(
        mln_runtime_event_style_image_missing* payload
    ) => new(CopyUtf8(payload->image_id, payload->image_id_size));

    private static RuntimeEventPayload.TileAction ReadTileAction(
        mln_runtime_event_tile_action* payload
    ) =>
        new(
            (TileOperation)payload->operation,
            payload->operation,
            TileId(payload->tile_id),
            CopyUtf8(payload->source_id, payload->source_id_size)
        );

    private static RuntimeEventPayload.OfflineRegionStatusChanged ReadOfflineRegionStatus(
        mln_runtime_event_offline_region_status* payload
    ) => new(payload->region_id, OfflineRegionStatus(payload->status));

    private static RuntimeEventPayload.OfflineRegionResponseError ReadOfflineRegionResponseError(
        mln_runtime_event_offline_region_response_error* payload
    ) => new(payload->region_id, (ResourceErrorReason)payload->reason, payload->reason);

    private static RuntimeEventPayload.OfflineRegionTileCountLimit ReadOfflineRegionTileCountLimit(
        mln_runtime_event_offline_region_tile_count_limit* payload
    ) => new(payload->region_id, payload->limit);

    private static RuntimeEventPayload.OfflineOperationCompleted ReadOfflineOperationCompleted(
        mln_runtime_event_offline_operation_completed* payload
    ) =>
        new(
            payload->operation_id,
            (OfflineOperationKind)payload->operation_kind,
            payload->operation_kind,
            (OfflineOperationResultKind)payload->result_kind,
            payload->result_kind,
            payload->result_status,
            payload->found != 0
        );

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

    internal static mln_runtime_event EmptyNativeEvent()
    {
        return new mln_runtime_event { size = (uint)Unsafe.SizeOf<mln_runtime_event>() };
    }
}
