// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Query;

public abstract record RenderedQueryGeometry
{
    private RenderedQueryGeometry() { }

    public sealed record Point(ScreenPoint Value) : RenderedQueryGeometry;

    public sealed record Box(ScreenBox Value) : RenderedQueryGeometry;

    public sealed record LineString(ScreenLineString Value) : RenderedQueryGeometry;

    public sealed record Unknown : RenderedQueryGeometry
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
