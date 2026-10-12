// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Region data delivered by an offline completion. Copied result, owned by the
/// caller. Copied result, owned by the caller. Copied result, owned by the
/// caller. Copied result, owned by the caller. Copied result, owned by the
/// caller.
/// </summary>
/// <remarks>
/// See <c>mln_offline_region_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
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

    /// <summary>
    /// Metadata bytes. Copied bytes, owned by the caller.
    /// </summary>
    public byte[] Metadata
    {
        get => MetadataStorage.ToArray();
        init => MetadataStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> MetadataStorage { get; init; }
}
