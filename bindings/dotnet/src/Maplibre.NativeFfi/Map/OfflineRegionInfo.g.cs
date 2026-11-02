// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An offline region in the runtime's cache file.
/// </summary>
/// <remarks>
/// See <c>mln_offline_region_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OfflineRegionInfo
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OfflineRegionInfo(
        long Id,
        OfflineRegionDefinition Definition,
        ImmutableArray<byte> Metadata
    )
    {
        this.Id = Id;
        this.Definition = Definition;
        this.Metadata = Metadata;
    }

    public long Id { get; init; }
    public required OfflineRegionDefinition Definition { get; init; }
    public ImmutableArray<byte> Metadata
    {
        get => MetadataStorage.AsImmutable();
        init => MetadataStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> MetadataStorage { get; init; }
}
