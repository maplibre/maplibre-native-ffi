// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record ResourceRequestInfo
{
    /// <summary>
    /// URL entering the network layer, before tile server normalization.
    /// </summary>
    public string? RequestedUrl { get; set; }

    /// <summary>
    /// URL to fetch, after resource-kind normalization against the runtime's
    /// tile server options and API key.
    /// </summary>
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
