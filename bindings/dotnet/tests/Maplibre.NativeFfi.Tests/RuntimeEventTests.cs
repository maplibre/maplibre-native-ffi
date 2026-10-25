using System.Runtime.InteropServices;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

public sealed unsafe class RuntimeEventTests
{
    [Fact]
    public void EventCopyRejectsInvalidStrideAndMessageViews()
    {
        var record = new mln_runtime_event { message = new mln_buffer_view { size = 2 } };
        var batch = new mln_event_batch_view
        {
            event_size = (uint)sizeof(mln_runtime_event) - 1,
            events = &record,
            event_count = 1,
        };
        Assert.Throws<InvalidOperationException>(() =>
            GeneratedValues.CopyEventBatchViewEvents(batch)
        );
        batch.event_size++;
        Assert.Throws<InvalidOperationException>(() =>
            GeneratedValues.CopyEventBatchViewEvents(batch)
        );
    }

    private static int OffsetOf(string fieldName) =>
        Marshal.OffsetOf<mln_runtime_event>(fieldName).ToInt32();

    [Fact]
    public void CopiesEveryMessageAndTypedPayloadAtTheBatchStride()
    {
        using var arena = new EventBatches.MessageArena();
        var renderError = arena.Add("render failed");
        var sourceId = arena.Add("source-1");
        var events = new[]
        {
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_RENDER_ERROR,
                source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
                payload_type = (uint)mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_NONE,
                message = renderError,
            },
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
                source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
                payload_type = (uint)
                    mln_runtime_event_payload_type.MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION,
                message = sourceId,
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
        };

        // A batch from a newer library steps by a wider stride than this binding's record.
        var copied = EventBatches.Decode(events, EventBatches.Stride + 24);

        Assert.Equal(
            [RuntimeEventType.MapRenderError, RuntimeEventType.MapTileAction],
            copied.Select(runtimeEvent => runtimeEvent.Type)
        );
        Assert.Equal("render failed", copied[0].Message);
        Assert.IsType<RuntimeEvent.PayloadValue.None>(copied[0].Payload);

        // The tile action carries its source ID as the event message.
        Assert.Equal("source-1", copied[1].Message);
        var tileAction = Assert.IsType<RuntimeEvent.PayloadValue.TileAction>(copied[1].Payload);
        Assert.Equal(TileOperation.EndParse, tileAction.Value.Operation);
        Assert.Equal(new TileId(4, -1, 3, 2, 1), tileAction.Value.TileId);
    }

    [Fact]
    public void UnknownEventDomainsKeepRawValuesAndCopyThePayloadWindow()
    {
        var stride = EventBatches.Stride + 8;
        var payloadOffset = OffsetOf(nameof(mln_runtime_event.payload));
        var records = new byte[stride];
        BitConverter.GetBytes(4242u).CopyTo(records, OffsetOf(nameof(mln_runtime_event.type)));
        BitConverter.GetBytes(77u).CopyTo(records, OffsetOf(nameof(mln_runtime_event.source_type)));
        BitConverter
            .GetBytes(0x0700_0000_0000_0021UL)
            .CopyTo(records, OffsetOf(nameof(mln_runtime_event.source)));
        BitConverter
            .GetBytes(0x0000_0001_0000_0005UL)
            .CopyTo(records, OffsetOf(nameof(mln_runtime_event.generation)));
        BitConverter
            .GetBytes(999u)
            .CopyTo(records, OffsetOf(nameof(mln_runtime_event.payload_type)));
        for (var index = payloadOffset; index < records.Length; index++)
        {
            records[index] = (byte)(index - payloadOffset + 1);
        }

        var copied = EventBatches.DecodeRecordBytes(records, 1, stride);

        var runtimeEvent = Assert.Single(copied);
        Assert.Equal(4242u, (uint)runtimeEvent.Type);
        Assert.Equal((RuntimeEventType)4242u, runtimeEvent.Type);
        Assert.Equal(77u, (uint)runtimeEvent.SourceType);
        Assert.Equal((RuntimeEventSourceType)77u, runtimeEvent.SourceType);
        Assert.Equal(0x0700_0000_0000_0021UL, runtimeEvent.Source);
        Assert.Equal(0x0000_0001_0000_0005UL, runtimeEvent.Generation);

        // The window is the batch stride minus the payload offset, so it grows with a stride
        // a later library version widens.
        var unknown = Assert.IsType<RuntimeEvent.PayloadValue.Unknown>(runtimeEvent.Payload);
        Assert.Equal(999u, unknown.Tag);
        Assert.Equal(records.AsSpan(payloadOffset).ToArray(), unknown.PayloadBytes);

        records[payloadOffset] = 0xFF;
        Assert.Equal(1, unknown.PayloadBytes[0]);
    }

    [Fact]
    public void NumbersAreCheckedWhenNarrowedAndKeptWhenOpen()
    {
        // A batch count that no managed array can index fails instead of wrapping around.
        Assert.Throws<OverflowException>(() =>
            EventBatches.DecodeRecordBytes([], (nuint)int.MaxValue + 1, EventBatches.Stride)
        );

        var events = new[]
        {
            new mln_runtime_event
            {
                type = (uint)mln_runtime_event_type.MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
                source_type = (uint)mln_runtime_event_source_type.MLN_RUNTIME_EVENT_SOURCE_MAP,
                source = ulong.MaxValue,
                code = int.MinValue,
            },
            new mln_runtime_event { type = uint.MaxValue },
        };
        var copied = EventBatches.Decode(events, EventBatches.Stride);

        Assert.Equal(ulong.MaxValue, copied[0].Source);
        Assert.Equal(int.MinValue, copied[0].Code);
        Assert.Equal(uint.MaxValue, (uint)copied[1].Type);
    }
}
