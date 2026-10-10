using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Internal.Struct;

namespace Maplibre.NativeFfi.Tests;

/// <summary>Event batches laid out by hand, for the decoder that copies drained batches.</summary>
internal static unsafe class EventBatches
{
    /// <summary>The event stride this binding compiled against.</summary>
    internal static uint Stride => (uint)Unsafe.SizeOf<mln_runtime_event>();

    /// <summary>
    /// Decodes events laid out at a caller-chosen stride, so a decoder that indexed events by its
    /// own record size reads the wrong bytes.
    /// </summary>
    internal static IReadOnlyList<RuntimeEvent> Decode(
        mln_runtime_event[] events,
        byte[] messages,
        uint eventSize
    )
    {
        var records = new byte[checked(events.Length * (int)eventSize)];
        for (var index = 0; index < events.Length; index++)
            MemoryMarshal.Write(records.AsSpan(index * (int)eventSize), in events[index]);
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
            var batch = new mln_event_batch_view
            {
                size = (uint)Unsafe.SizeOf<mln_event_batch_view>(),
                event_size = eventSize,
                events = (mln_runtime_event*)recordBytes,
                event_count = eventCount,
                messages = (sbyte*)messageBytes,
                messages_size = (nuint)messages.Length,
            };
            return GeneratedValues.CopyEventBatchView(batch).Events;
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
