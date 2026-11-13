// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An event that a runtime or one of its maps raised.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Source">
/// The runtime or map that raised the event, as <c>source_type</c> says.
/// </param>
/// <param name="Generation">
/// For a map event, a map snapshot with this generation or a later one includes
/// what the event reports. Zero for a runtime event.
/// </param>
/// <param name="Message">
/// The error's text for an event that reports a failure, or empty.
/// </param>
public readonly partial record struct RuntimeEvent(
    RuntimeEventType Type,
    RuntimeEventSourceType SourceType,
    ulong Source,
    ulong Generation,
    string Message,
    RuntimeEvent.PayloadValue Payload
)
{
    public abstract record PayloadValue
    {
        private PayloadValue() { }

        public sealed record CameraChange(RuntimeEventCameraChange Value) : PayloadValue;

        public sealed record LoadingFailed(RuntimeEventLoadingFailed Value) : PayloadValue;

        public sealed record RenderFrame(RuntimeEventRenderFrame Value) : PayloadValue;

        public sealed record StyleImage(RuntimeEventStyleImage Value) : PayloadValue;

        public sealed record TileAction(RuntimeEventTileAction Value) : PayloadValue;

        public sealed record SourceChanged(RuntimeEventSourceChanged Value) : PayloadValue;

        public sealed record OfflineRegionStatus(RuntimeEventOfflineRegionStatus Value)
            : PayloadValue;

        public sealed record OfflineRegionResponseError(
            RuntimeEventOfflineRegionResponseError Value
        ) : PayloadValue;

        public sealed record Glyphs(RuntimeEventGlyphs Value) : PayloadValue;

        public sealed record Sprite(RuntimeEventSprite Value) : PayloadValue;

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
