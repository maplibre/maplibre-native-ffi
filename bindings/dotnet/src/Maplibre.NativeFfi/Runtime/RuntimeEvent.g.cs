// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One delivered runtime event.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Type">
/// One of <c>mln_runtime_event_type</c>.
/// </param>
/// <param name="SourceType">
/// One of <c>mln_runtime_event_source_type</c>.
/// </param>
/// <param name="Source">
/// Source handle selected by source_type: an <c>mln_runtime</c> or an
/// <c>mln_map</c>.
/// </param>
/// <param name="Generation">
/// Map snapshot generation that the event reports, or zero when source_type is
/// <c>MLN_RUNTIME_EVENT_SOURCE_RUNTIME</c>.
/// </param>
/// <param name="Code">
/// Secondary event detail whose meaning type selects; see
/// <c>mln_runtime_event_type</c>.
/// </param>
/// <param name="Message">
/// Borrowed UTF-8 message bytes, empty when this event carries no message.
/// </param>
/// <param name="Payload">
/// Typed payload selected by payload_type.
/// </param>
public readonly partial record struct RuntimeEvent(
    RuntimeEventType Type,
    RuntimeEventSourceType SourceType,
    ulong Source,
    ulong Generation,
    int Code,
    string Message,
    RuntimeEvent.PayloadValue Payload
)
{
    public abstract record PayloadValue
    {
        private PayloadValue() { }

        public sealed record RenderFrame(RuntimeEventRenderFrame Value) : PayloadValue;

        public sealed record RenderMap(RuntimeEventRenderMap Value) : PayloadValue;

        public sealed record TileAction(RuntimeEventTileAction Value) : PayloadValue;

        public sealed record OfflineRegionStatus(RuntimeEventOfflineRegionStatus Value)
            : PayloadValue;

        public sealed record OfflineRegionResponseError(
            RuntimeEventOfflineRegionResponseError Value
        ) : PayloadValue;

        public sealed record OfflineRegionTileCountLimit(
            RuntimeEventOfflineRegionTileCountLimit Value
        ) : PayloadValue;

        public sealed record None : PayloadValue;

        public sealed record Unknown : PayloadValue
        {
            private readonly byte[] bytes;
            public uint Tag { get; }
            public byte[] PayloadBytes => (byte[])bytes.Clone();

            public Unknown(uint tag, byte[] payloadBytes)
            {
                Tag = tag;
                bytes = (byte[])payloadBytes.Clone();
            }

            public bool Equals(Unknown? other) =>
                other is not null && Tag == other.Tag && bytes.AsSpan().SequenceEqual(other.bytes);

            public override int GetHashCode()
            {
                var hash = new HashCode();
                hash.Add(Tag);
                foreach (var value in bytes)
                    hash.Add(value);
                return hash.ToHashCode();
            }
        }
    }
}
