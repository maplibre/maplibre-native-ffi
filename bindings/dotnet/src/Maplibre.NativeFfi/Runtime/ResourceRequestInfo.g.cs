// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record ResourceRequestInfo
{
    /// <summary>
    /// URL entering the network layer, before tile server normalization.
    /// </summary>
    public required string RequestedUrl { get; init; }

    /// <summary>
    /// URL to fetch, after resource-kind normalization against the runtime's
    /// tile server options and API key.
    /// </summary>
    public required string ResolvedUrl { get; init; }
    public ResourceKind Kind { get; init; }
    public ResourceLoadingMethod LoadingMethod { get; init; }
    public ResourcePriority Priority { get; init; }
    public ResourceUsage Usage { get; init; }
    public ResourceStoragePolicy StoragePolicy { get; init; }
    public ResourceRange? Range { get; init; }
    public long? PriorModifiedUnixMs { get; init; }
    public long? PriorExpiresUnixMs { get; init; }

    /// <summary>
    /// Entity tag of the cached copy, or empty when there is none.
    /// </summary>
    public required string PriorEtag { get; init; }
    public ImmutableArray<byte> PriorData
    {
        get => PriorDataStorage.AsImmutable();
        init => PriorDataStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> PriorDataStorage { get; init; }
}
