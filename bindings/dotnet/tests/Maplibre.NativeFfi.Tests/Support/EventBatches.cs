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
    internal static IReadOnlyList<RuntimeEvent> Decode(mln_runtime_event[] events, uint eventSize)
    {
        var records = new byte[checked(events.Length * (int)eventSize)];
        for (var index = 0; index < events.Length; index++)
            MemoryMarshal.Write(records.AsSpan(index * (int)eventSize), in events[index]);
        return DecodeRecordBytes(records, (nuint)events.Length, eventSize);
    }

    /// <summary>Decodes a batch whose event records the caller laid out byte by byte.</summary>
    internal static IReadOnlyList<RuntimeEvent> DecodeRecordBytes(
        byte[] records,
        nuint eventCount,
        uint eventSize
    )
    {
        fixed (byte* recordBytes = records)
        {
            var batch = new mln_event_batch_view
            {
                size = (uint)Unsafe.SizeOf<mln_event_batch_view>(),
                event_size = eventSize,
                events = (mln_runtime_event*)recordBytes,
                event_count = eventCount,
            };
            return GeneratedValues.CopyEventBatchView(batch).Events;
        }
    }

    /// <summary>Owns message views until the decoder copies their bytes.</summary>
    internal sealed class MessageArena : IDisposable
    {
        private readonly List<nint> allocations = [];

        internal mln_buffer_view Add(string message)
        {
            var encoded = Encoding.UTF8.GetBytes(message);
            var data = (byte*)NativeMemory.Alloc((nuint)encoded.Length);
            encoded.CopyTo(new Span<byte>(data, encoded.Length));
            allocations.Add((nint)data);
            return new mln_buffer_view { data = data, size = (nuint)encoded.Length };
        }

        public void Dispose()
        {
            foreach (var data in allocations)
                NativeMemory.Free((void*)data);
        }
    }
}
