using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Offline;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Resource;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class RuntimeEventTests
{
    private static int OffsetOf(string fieldName) =>
        Marshal.OffsetOf<mln_runtime_event>(fieldName).ToInt32();

    [BindingSpecTest("BND-082", "BND-090")]
    [Fact]
    public void CopiesEveryMessageAndTypedPayloadOfOneBatch()
    {
        var arena = new RuntimeEventTestHelpers.MessageArena();
        var renderError = arena.Add("render failed");
        var sourceId = arena.Add("source-1");
        var events = new[]
        {
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_RENDER_ERROR,
                source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
                payload_type = (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_NONE,
                message_offset = renderError.Offset,
                message_size = renderError.Size,
            },
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
                source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
                payload_type = (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
                message_offset = sourceId.Offset,
                message_size = sourceId.Size,
                payload = new mln_runtime_event_payload
                {
                    tile_action = new mln_runtime_event_tile_action
                    {
                        operation = (uint)mln_tile_operation.MLN_TILE_OPERATION_END_PARSE,
                        tile_id = new mln_tile_id
                        {
                            overscaled_z = 4,
                            wrap = -1,
                            canonical_z = 3,
                            canonical_x = 2,
                            canonical_y = 1,
                        },
                    },
                },
            },
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
                source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
                payload_type = (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP,
                payload = new mln_runtime_event_payload
                {
                    render_map = new mln_runtime_event_render_map
                    {
                        mode = (uint)mln_render_mode.MLN_RENDER_MODE_FULL,
                    },
                },
            },
        };

        var copied = RuntimeEventTestHelpers.DecodeBatch(
            events,
            arena.Bytes,
            RuntimeEventTestHelpers.EventStride
        );

        Assert.Equal(
            [
                RuntimeEventType.MapRenderError,
                RuntimeEventType.MapTileAction,
                RuntimeEventType.MapRenderMapFinished,
            ],
            copied.Select(runtimeEvent => runtimeEvent.Type)
        );
        Assert.Equal("render failed", copied[0].Message);
        Assert.Same(RuntimeEventPayload.None.Instance, copied[0].Payload);

        // The tile action carries its source ID as the event message.
        Assert.Equal("source-1", copied[1].Message);
        var tileAction = Assert.IsType<RuntimeEventPayload.TileAction>(copied[1].Payload);
        Assert.Equal(TileOperation.EndParse, tileAction.Operation);
        Assert.Equal(new TileId(4, -1, 3, 2, 1), tileAction.TileId);

        Assert.Equal(string.Empty, copied[2].Message);
        var renderMap = Assert.IsType<RuntimeEventPayload.RenderMap>(copied[2].Payload);
        Assert.Equal(RenderMode.Full, renderMap.Mode);
    }

    [BindingSpecTest("BND-087")]
    [Fact]
    public void WalksEventsByTheStrideTheBatchReports()
    {
        var stride = RuntimeEventTestHelpers.EventStride + 24;
        var events = new[]
        {
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
                code = 1,
            },
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_IDLE,
                code = 2,
            },
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED,
                code = 3,
            },
        };

        var copied = RuntimeEventTestHelpers.DecodeBatch(events, [], stride);

        Assert.Equal(
            [
                RuntimeEventType.MapCameraDidChange,
                RuntimeEventType.MapIdle,
                RuntimeEventType.MapLoadingFinished,
            ],
            copied.Select(runtimeEvent => runtimeEvent.Type)
        );
        Assert.Equal([1, 2, 3], copied.Select(runtimeEvent => runtimeEvent.Code));
    }

    [BindingSpecTest("BND-083")]
    [Fact]
    public void UnknownEventDomainsKeepRawValuesAndCopyThePayloadWindow()
    {
        var stride = RuntimeEventTestHelpers.EventStride + 8;
        var payloadOffset = OffsetOf(nameof(mln_runtime_event.payload));
        var records = new byte[stride];
        BitConverter.GetBytes(4242u).CopyTo(records, OffsetOf(nameof(mln_runtime_event.type)));
        BitConverter.GetBytes(77u).CopyTo(records, OffsetOf(nameof(mln_runtime_event.source_type)));
        BitConverter
            .GetBytes(0x0700_0000_0000_0021UL)
            .CopyTo(records, OffsetOf(nameof(mln_runtime_event.source)));
        BitConverter
            .GetBytes(999u)
            .CopyTo(records, OffsetOf(nameof(mln_runtime_event.payload_type)));
        for (var index = payloadOffset; index < records.Length; index++)
        {
            records[index] = (byte)(index - payloadOffset + 1);
        }

        var copied = RuntimeEventTestHelpers.DecodeRecordBytes(records, [], 1, stride);

        var runtimeEvent = Assert.Single(copied);
        Assert.Equal(4242u, runtimeEvent.RawType);
        Assert.Equal((RuntimeEventType)4242u, runtimeEvent.Type);
        Assert.Equal(77u, runtimeEvent.RawSourceType);
        Assert.Equal((RuntimeEventSourceType)77u, runtimeEvent.SourceType);
        Assert.Equal(0x0700_0000_0000_0021UL, runtimeEvent.RawSource);
        Assert.Null(runtimeEvent.MapSource);
        Assert.Null(runtimeEvent.RuntimeSource);
        Assert.Equal(999u, runtimeEvent.RawPayloadType);

        // The window is the batch stride minus the payload offset, so it grows with a stride
        // a later library version widens.
        var unknown = Assert.IsType<RuntimeEventPayload.Unknown>(runtimeEvent.Payload);
        Assert.Equal(999u, unknown.RawPayloadType);
        Assert.Equal(records.AsSpan(payloadOffset).ToArray(), unknown.PayloadBytes);

        records[payloadOffset] = 0xFF;
        Assert.Equal(1, unknown.PayloadBytes[0]);
    }

    [BindingSpecTest("BND-069", "BND-083")]
    [Fact]
    public void UnknownRuntimePayloadSnapshotsBytesAndReturnsCopies()
    {
        var source = new byte[] { 1, 2, 3 };
        var payload = new RuntimeEventPayload.Unknown(999, source);
        source[0] = 9;

        var first = payload.PayloadBytes;
        Assert.Equal([1, 2, 3], first);
        first[0] = 8;
        Assert.Equal([1, 2, 3], payload.PayloadBytes);
    }

    [BindingSpecTest("BND-086")]
    [Fact]
    public void UnmatchedMapSourceKeepsItsRawIdentityAndExposesNoPublicMap()
    {
        var source = SyntheticHandles.Map(1234).Value;
        var events = new[]
        {
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
                source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_MAP,
                source = source,
            },
        };

        var copied = RuntimeEventTestHelpers.DecodeBatch(
            events,
            [],
            RuntimeEventTestHelpers.EventStride
        );

        var runtimeEvent = Assert.Single(copied);
        Assert.Equal(RuntimeEventSourceType.Map, runtimeEvent.SourceType);
        Assert.Equal(source, runtimeEvent.RawSource);
        Assert.Null(runtimeEvent.MapSource);
        Assert.Null(runtimeEvent.RuntimeSource);
    }

    [BindingSpecTest("BND-085")]
    [Fact]
    public void OfflineRegionObservationEventsMaterializeCopiedPublicPayloads()
    {
        var arena = new RuntimeEventTestHelpers.MessageArena();
        var errorText = arena.Add("not found");
        var events = new[]
        {
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
                payload_type = (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS,
                payload = new mln_runtime_event_payload
                {
                    offline_region_status = new mln_runtime_event_offline_region_status
                    {
                        region_id = 42,
                        status = new mln_offline_region_status
                        {
                            download_state = (uint)
                                mln_offline_region_download_state.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
                            completed_resource_count = 1,
                            completed_resource_size = 2,
                            completed_tile_count = 3,
                            required_tile_count = 4,
                            completed_tile_size = 5,
                            required_resource_count = 6,
                            required_resource_count_is_precise = 1,
                            complete = 1,
                        },
                    },
                },
            },
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
                payload_type = (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR,
                message_offset = errorText.Offset,
                message_size = errorText.Size,
                payload = new mln_runtime_event_payload
                {
                    offline_region_response_error =
                        new mln_runtime_event_offline_region_response_error
                        {
                            region_id = 42,
                            reason = (uint)ResourceErrorReason.NotFound,
                        },
                },
            },
        };

        var copied = RuntimeEventTestHelpers.DecodeBatch(
            events,
            arena.Bytes,
            RuntimeEventTestHelpers.EventStride
        );

        var status = Assert.IsType<RuntimeEventPayload.OfflineRegionStatusChanged>(
            copied[0].Payload
        );
        Assert.Equal(42, status.RegionId);
        Assert.Equal(OfflineRegionDownloadState.Active, status.Status.DownloadState);
        Assert.Equal(6u, status.Status.RequiredResourceCount);
        Assert.True(status.Status.RequiredResourceCountIsPrecise);
        Assert.True(status.Status.Complete);

        var responseError = Assert.IsType<RuntimeEventPayload.OfflineRegionResponseError>(
            copied[1].Payload
        );
        Assert.Equal(42, responseError.RegionId);
        Assert.Equal(ResourceErrorReason.NotFound, responseError.Reason);
        Assert.Equal((uint)ResourceErrorReason.NotFound, responseError.RawReason);
        Assert.Equal("not found", copied[1].Message);
    }
}
