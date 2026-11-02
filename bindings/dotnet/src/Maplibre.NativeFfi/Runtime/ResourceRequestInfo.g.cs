// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A request that a resource provider serves.
/// </summary>
/// <remarks>
/// <para>When the cache holds an earlier copy of the resource, the
/// <c>prior_</c> fields describe it.</para>
/// <para>See <c>mln_resource_request_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public sealed record ResourceRequestInfo
{
    /// <summary>
    /// The URL as the style names it.
    /// </summary>
    public string RequestedUrl { get; init; } = "";

    /// <summary>
    /// The URL to load, after the library expands any shorthand in
    /// <c>requested_url</c> and before any resource transform. Routes match
    /// this URL.
    /// </summary>
    public string ResolvedUrl { get; init; } = "";
    public ResourceKind Kind { get; init; }
    public ResourceLoadingMethod LoadingMethod { get; init; }
    public ResourcePriority Priority { get; init; }
    public ResourceUsage Usage { get; init; }
    public ResourceStoragePolicy StoragePolicy { get; init; }

    /// <summary>
    /// The only bytes of the resource that the request wants.
    /// </summary>
    public ResourceRange? Range { get; init; }
    public long? PriorModifiedUnixMs { get; init; }
    public long? PriorExpiresUnixMs { get; init; }

    /// <summary>
    /// Empty when the earlier copy has no ETag.
    /// </summary>
    public string PriorEtag { get; init; } = "";
    public ImmutableArray<byte> PriorData
    {
        get => PriorDataStorage.AsImmutable();
        init => PriorDataStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> PriorDataStorage { get; init; }
}
