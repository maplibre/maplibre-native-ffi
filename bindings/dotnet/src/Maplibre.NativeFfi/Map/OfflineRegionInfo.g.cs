// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

public readonly record struct OfflineRegionInfo
{
    public OfflineRegionInfo(long Id, OfflineRegionDefinition Definition, byte[] Metadata)
        : this(Id, Definition, Metadata, false) { }

    internal OfflineRegionInfo(
        long Id,
        OfflineRegionDefinition Definition,
        byte[] Metadata,
        bool adopt
    )
    {
        this.Id = Id;
        this.Definition = Definition;
        this.storageMetadata = adopt ? Metadata : Metadata?.ToArray() ?? [];
    }

    public long Id { get; init; }
    public OfflineRegionDefinition Definition { get; init; }
    private readonly byte[]? storageMetadata;
    public byte[] Metadata
    {
        get => storageMetadata?.ToArray() ?? [];
        init => storageMetadata = value?.ToArray() ?? [];
    }
    internal byte[] MetadataStorage
    {
        get => storageMetadata ?? [];
        init => storageMetadata = value;
    }

    public bool Equals(OfflineRegionInfo other) =>
        EqualityComparer<long>.Default.Equals(Id, other.Id)
        && EqualityComparer<OfflineRegionDefinition>.Default.Equals(Definition, other.Definition)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            MetadataStorage,
            other.MetadataStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Id);
        hash.Add(Definition);
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(MetadataStorage)
        );
        return hash.ToHashCode();
    }
}
