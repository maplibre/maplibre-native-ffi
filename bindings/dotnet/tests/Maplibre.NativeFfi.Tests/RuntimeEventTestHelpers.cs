using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Maplibre.NativeFfi.Error;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Runtime;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

internal static class TestHandles
{
    internal static RuntimeHandle CreateRuntime(RuntimeOptions options) =>
        RuntimeHandle.CreateAsync(options).GetAwaiter().GetResult();

    internal static MapHandle CreateMap(RuntimeHandle runtime, MapOptions options) =>
        MapHandle.CreateAsync(runtime, options).GetAwaiter().GetResult();

    internal static void Close(RuntimeHandle runtime) =>
        runtime.CloseAsync().GetAwaiter().GetResult();

    internal static void Close(MapHandle map) => map.CloseAsync().GetAwaiter().GetResult();
}

internal static unsafe class RuntimeEventTestHelpers
{
    /// <summary>The event stride this binding compiled against.</summary>
    internal static uint EventStride => (uint)Unsafe.SizeOf<mln_runtime_event>();

    internal static RuntimeEvent WaitForMapEvent(
        RuntimeHandle runtime,
        MapHandle map,
        RuntimeEventType eventType
    )
    {
        for (var attempt = 0; attempt < 1000; attempt++)
        {
            foreach (var runtimeEvent in runtime.DrainEvents().Events)
            {
                if (
                    runtimeEvent.Type == eventType
                    && runtimeEvent.SourceType == RuntimeEventSourceType.Map
                    && ReferenceEquals(runtimeEvent.MapSource, map)
                )
                {
                    return runtimeEvent;
                }
            }

            Thread.Sleep(1);
        }

        throw new TimeoutException($"Timed out waiting for map event {eventType}.");
    }

    internal static RuntimeEvent WaitForCommand(RuntimeHandle runtime, ulong commandId)
    {
        for (var attempt = 0; attempt < 5000; attempt++)
        {
            foreach (var runtimeEvent in runtime.DrainEvents().Events)
            {
                if (
                    runtimeEvent.Payload is RuntimeEventPayload.CommandFinished command
                    && command.CommandId == commandId
                )
                {
                    return runtimeEvent;
                }
            }
            Thread.Sleep(1);
        }
        throw new TimeoutException($"Timed out waiting for command {commandId}.");
    }

    /// <summary>
    /// Waits for the command to finish, asserts its disposition matches the expected status
    /// (committed for Ok with a nonzero generation, failed with the status code and a message
    /// otherwise), and returns the payload.
    /// </summary>
    internal static RuntimeEventPayload.CommandFinished AssertCommandFinishes(
        RuntimeHandle runtime,
        ulong commandId,
        MaplibreStatus expectedStatus
    )
    {
        Assert.NotEqual(0ul, commandId);
        var finished = WaitForCommand(runtime, commandId);
        var completion = Assert.IsType<RuntimeEventPayload.CommandFinished>(finished.Payload);
        if (expectedStatus == MaplibreStatus.Ok)
        {
            Assert.Equal(CommandDisposition.Committed, completion.Disposition);
            Assert.NotEqual(0ul, completion.Generation);
        }
        else
        {
            Assert.Equal(CommandDisposition.Failed, completion.Disposition);
            Assert.Equal((int)expectedStatus, finished.Code);
            Assert.NotEmpty(finished.Message);
        }
        return completion;
    }

    /// <summary>Drains until one batch reports no events.</summary>
    internal static List<RuntimeEvent> DrainUntilIdle(RuntimeHandle runtime)
    {
        var events = new List<RuntimeEvent>();
        for (var attempt = 0; attempt < 100; attempt++)
        {
            Thread.Sleep(1);
            var batch = runtime.DrainEvents();
            if (batch.Events.Count == 0)
            {
                return events;
            }

            events.AddRange(batch.Events);
        }

        throw new TimeoutException("The runtime kept producing events while idle.");
    }

    /// <summary>
    /// Decodes the batch a drain would hand the decoder, at a caller-chosen stride, so a decode
    /// that indexed events by its own record size reads the wrong bytes.
    /// </summary>
    internal static IReadOnlyList<RuntimeEvent> DecodeBatch(
        mln_runtime_event[] events,
        byte[] messages,
        uint eventSize
    )
    {
        var records = new byte[checked(events.Length * (int)eventSize)];
        for (var index = 0; index < events.Length; index++)
        {
            MemoryMarshal.Write(records.AsSpan(index * (int)eventSize), in events[index]);
        }
        return DecodeRecordBytes(records, messages, (nuint)events.Length, eventSize);
    }

    /// <summary>Decodes a batch whose event records the caller laid out byte by byte.</summary>
    internal static IReadOnlyList<RuntimeEvent> DecodeRecordBytes(
        byte[] records,
        byte[] messages,
        nuint eventCount,
        uint eventSize
    )
    {
        fixed (byte* recordBytes = records)
        fixed (byte* messageBytes = messages)
        {
            var batch = new mln_runtime_event_batch_view
            {
                size = (uint)Unsafe.SizeOf<mln_runtime_event_batch_view>(),
                event_size = eventSize,
                events = (mln_runtime_event*)recordBytes,
                event_count = eventCount,
                messages = (sbyte*)messageBytes,
                messages_size = (nuint)messages.Length,
            };
            return RuntimeStructs.ReadBatch(batch);
        }
    }

    /// <summary>
    /// A message arena laid out the way a drain lays one out: every message is followed by a null
    /// terminator, and each event records its own offset into the bytes.
    /// </summary>
    internal sealed class MessageArena
    {
        private readonly List<byte> bytes = [];

        internal (uint Offset, uint Size) Add(string message)
        {
            var encoded = Encoding.UTF8.GetBytes(message);
            var offset = (uint)bytes.Count;
            bytes.AddRange(encoded);
            bytes.Add(0);
            return (offset, (uint)encoded.Length);
        }

        internal byte[] Bytes => [.. bytes];
    }
}
