// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Runtime;

public readonly partial record struct RuntimeEvent(
    RuntimeEventType Type,
    RuntimeEventSourceType SourceType,
    ulong Source,
    int Code,
    RuntimeEvent.PayloadValue Payload,
    string Message
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

        public sealed record CameraTransitionFinished(RuntimeEventCameraTransitionFinished Value)
            : PayloadValue;

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
