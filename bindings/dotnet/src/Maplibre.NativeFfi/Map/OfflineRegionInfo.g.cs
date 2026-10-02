// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public readonly record struct OfflineRegionInfo
{
    public OfflineRegionInfo(long Id, OfflineRegionDefinition Definition, byte[] Metadata)
    {
        this.Id = Id;
        this.Definition = Definition;
        this.Metadata = Metadata;
    }

    public long Id { get; init; }
    public OfflineRegionDefinition Definition { get; init; }
    public byte[] Metadata
    {
        get => MetadataStorage.ToArray();
        init => MetadataStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> MetadataStorage { get; init; }
}
