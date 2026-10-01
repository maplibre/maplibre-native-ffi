// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public sealed record ResourceRequest
{
    public string? RequestedUrl { get; set; }
    public string? ResolvedUrl { get; set; }
    public ResourceKind Kind { get; set; }
    public ResourceLoadingMethod LoadingMethod { get; set; }
    public ResourcePriority Priority { get; set; }
    public ResourceUsage Usage { get; set; }
    public ResourceStoragePolicy StoragePolicy { get; set; }
    public ResourceRequest.RangeValue? Range { get; set; }
    public long? PriorModifiedUnixMs { get; set; }
    public long? PriorExpiresUnixMs { get; set; }
    public string? PriorEtag { get; set; }
    private byte[]? storagePriorData;
    public byte[] PriorData
    {
        get => storagePriorData?.ToArray() ?? [];
        set => storagePriorData = value?.ToArray() ?? [];
    }
    internal byte[] PriorDataStorage
    {
        get => storagePriorData ?? [];
        init => storagePriorData = value;
    }

    public bool Equals(ResourceRequest? other) =>
        other is not null
        && EqualityComparer<string?>.Default.Equals(RequestedUrl, other.RequestedUrl)
        && EqualityComparer<string?>.Default.Equals(ResolvedUrl, other.ResolvedUrl)
        && EqualityComparer<ResourceKind>.Default.Equals(Kind, other.Kind)
        && EqualityComparer<ResourceLoadingMethod>.Default.Equals(
            LoadingMethod,
            other.LoadingMethod
        )
        && EqualityComparer<ResourcePriority>.Default.Equals(Priority, other.Priority)
        && EqualityComparer<ResourceUsage>.Default.Equals(Usage, other.Usage)
        && EqualityComparer<ResourceStoragePolicy>.Default.Equals(
            StoragePolicy,
            other.StoragePolicy
        )
        && EqualityComparer<ResourceRequest.RangeValue?>.Default.Equals(Range, other.Range)
        && EqualityComparer<long?>.Default.Equals(PriorModifiedUnixMs, other.PriorModifiedUnixMs)
        && EqualityComparer<long?>.Default.Equals(PriorExpiresUnixMs, other.PriorExpiresUnixMs)
        && EqualityComparer<string?>.Default.Equals(PriorEtag, other.PriorEtag)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            PriorDataStorage,
            other.PriorDataStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(RequestedUrl);
        hash.Add(ResolvedUrl);
        hash.Add(Kind);
        hash.Add(LoadingMethod);
        hash.Add(Priority);
        hash.Add(Usage);
        hash.Add(StoragePolicy);
        hash.Add(Range);
        hash.Add(PriorModifiedUnixMs);
        hash.Add(PriorExpiresUnixMs);
        hash.Add(PriorEtag);
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(PriorDataStorage)
        );
        return hash.ToHashCode();
    }

    public readonly record struct RangeValue(ulong RangeStart, ulong RangeEnd);
}
