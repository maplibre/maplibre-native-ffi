using System.Runtime.CompilerServices;
using System.Text;
using Maplibre.Native.Internal.C;
using Maplibre.Native.Internal.Struct;
using Maplibre.Native.Offline;
using Maplibre.Native.Render;
using Maplibre.Native.Resource;
using Maplibre.Native.Runtime;
using Xunit;

namespace Maplibre.Native.Tests;

public sealed unsafe class RuntimeEventTests
{
    [BindingSpecTest("BND-082")]
    [Fact]
    public void CopiesRuntimeEventMessageAndRenderPayload()
    {
        var message = Encoding.UTF8.GetBytes("hello");
        fixed (byte* messagePointer = message)
        {
            var payload = new mln_runtime_event_render_map
            {
                size = (uint)Unsafe.SizeOf<mln_runtime_event_render_map>(),
                mode = (uint)mln_render_mode.MLN_RENDER_MODE_FULL,
            };
            var raw = RuntimeStructs.EmptyNativeEvent();
            raw.type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED;
            raw.source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_RUNTIME;
            raw.payload_type = (uint)
                mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP;
            raw.payload = &payload;
            raw.payload_size = (nuint)Unsafe.SizeOf<mln_runtime_event_render_map>();
            raw.message = (sbyte*)messagePointer;
            raw.message_size = (nuint)message.Length;

            var copied = RuntimeStructs.ReadEvent(raw);

            Assert.Equal(RuntimeEventType.MapRenderMapFinished, copied.Type);
            Assert.Equal("hello", copied.Message);
            Assert.Null(copied.RuntimeSource);
            Assert.Null(copied.MapSource);
            var renderMap = Assert.IsType<RuntimeEventPayload.RenderMap>(copied.Payload);
            Assert.Equal(RenderMode.Full, renderMap.Mode);
            Assert.Equal((uint)mln_render_mode.MLN_RENDER_MODE_FULL, renderMap.RawMode);
        }
    }

    [BindingSpecTest("BND-083", "BND-087")]
    [Fact]
    public void UndersizedKnownPayloadBecomesUnknown()
    {
        var payload = new mln_runtime_event_render_frame
        {
            size = (uint)Unsafe.SizeOf<mln_runtime_event_render_frame>(),
            mode = (uint)mln_render_mode.MLN_RENDER_MODE_FULL,
        };
        var raw = RuntimeStructs.EmptyNativeEvent();
        raw.payload_type = (uint)
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME;
        raw.payload = &payload;
        raw.payload_size = 1;

        var copied = RuntimeStructs.ReadEvent(raw);

        var unknown = Assert.IsType<RuntimeEventPayload.Unknown>(copied.Payload);
        Assert.Equal(
            (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME,
            unknown.RawPayloadType
        );
        Assert.Equal(
            [unchecked((byte)Unsafe.SizeOf<mln_runtime_event_render_frame>())],
            unknown.PayloadBytes
        );
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
    public void UnmatchedMapSourceDoesNotExposePublicMapOrNativeIdentity()
    {
        var raw = RuntimeStructs.EmptyNativeEvent();
        raw.type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_LOADING_STARTED;
        raw.source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_MAP;
        raw.source = (void*)1234;

        var copied = RuntimeStructs.ReadEvent(raw, null, _ => null);

        Assert.Equal(RuntimeEventSourceType.Map, copied.SourceType);
        Assert.Null(copied.MapSource);
        Assert.Null(copied.RuntimeSource);
    }

    [BindingSpecTest("BND-085")]
    [Fact]
    public void OfflineRegionObservationEventsMaterializeCopiedPublicPayloads()
    {
        // Support invariant for BND-085: event materialization covers native
        // observation payloads deterministically while asserting public payload values.
        var statusPayload = new mln_runtime_event_offline_region_status
        {
            size = (uint)Unsafe.SizeOf<mln_runtime_event_offline_region_status>(),
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
        };
        var statusRaw = RuntimeStructs.EmptyNativeEvent();
        statusRaw.type = (uint)
            mln_runtime_event_type.MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED;
        statusRaw.payload_type = (uint)
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS;
        statusRaw.payload = &statusPayload;
        statusRaw.payload_size = (nuint)Unsafe.SizeOf<mln_runtime_event_offline_region_status>();

        var errorPayload = new mln_runtime_event_offline_region_response_error
        {
            size = (uint)Unsafe.SizeOf<mln_runtime_event_offline_region_response_error>(),
            region_id = 42,
            reason = (uint)ResourceErrorReason.NotFound,
        };
        var errorRaw = RuntimeStructs.EmptyNativeEvent();
        errorRaw.type = (uint)
            mln_runtime_event_type.MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR;
        errorRaw.payload_type = (uint)
            mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR;
        errorRaw.payload = &errorPayload;
        errorRaw.payload_size = (nuint)
            Unsafe.SizeOf<mln_runtime_event_offline_region_response_error>();

        var statusEvent = RuntimeStructs.ReadEvent(statusRaw);
        var errorEvent = RuntimeStructs.ReadEvent(errorRaw);

        var status = Assert.IsType<RuntimeEventPayload.OfflineRegionStatusChanged>(
            statusEvent.Payload
        );
        Assert.Equal(42, status.RegionId);
        Assert.Equal(OfflineRegionDownloadState.Active, status.Status.DownloadState);
        Assert.Equal(6u, status.Status.RequiredResourceCount);
        Assert.True(status.Status.RequiredResourceCountIsPrecise);
        Assert.True(status.Status.Complete);

        var responseError = Assert.IsType<RuntimeEventPayload.OfflineRegionResponseError>(
            errorEvent.Payload
        );
        Assert.Equal(42, responseError.RegionId);
        Assert.Equal(ResourceErrorReason.NotFound, responseError.Reason);
        Assert.Equal((uint)ResourceErrorReason.NotFound, responseError.RawReason);
    }

    [BindingSpecTest("BND-080")]
    [Fact]
    public void EmptyRuntimePollReturnsNull()
    {
        using var runtime = RuntimeHandle.Create(new RuntimeOptions());

        Assert.Null(runtime.PollEvent());
    }
}
