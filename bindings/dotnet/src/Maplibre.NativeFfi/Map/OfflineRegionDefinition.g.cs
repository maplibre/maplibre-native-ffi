// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Tagged offline region definition.
/// </summary>
/// <remarks>
/// See <c>mln_offline_region_definition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public abstract record OfflineRegionDefinition
{
    private OfflineRegionDefinition() { }

    public sealed record TilePyramid(OfflineTilePyramidRegionDefinition Value)
        : OfflineRegionDefinition;

    public sealed record Geometry(OfflineGeometryRegionDefinition Value) : OfflineRegionDefinition;

    public sealed record Unknown : OfflineRegionDefinition
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
