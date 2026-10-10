// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record ResourceRequest
{
    public string? RequestedUrl { get; set; }
    public string? ResolvedUrl { get; set; }
    public ResourceKind Kind { get; set; }
    public ResourceLoadingMethod LoadingMethod { get; set; }
    public ResourcePriority Priority { get; set; }
    public ResourceUsage Usage { get; set; }
    public ResourceStoragePolicy StoragePolicy { get; set; }
    public ResourceRange? Range { get; set; }
    public long? PriorModifiedUnixMs { get; set; }
    public long? PriorExpiresUnixMs { get; set; }
    public string? PriorEtag { get; set; }
    public byte[] PriorData
    {
        get => PriorDataStorage.ToArray();
        set => PriorDataStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> PriorDataStorage { get; set; }
}
