namespace Maplibre.Native.Resource;

public enum ResourceKind : uint { Unknown = 0, Style = 1, Source = 2, Tile = 3, Glyphs = 4, SpriteImage = 5, SpriteJson = 6, Image = 7 }
public enum ResourceLoadingMethod : uint { All = 0, CacheOnly = 1, NetworkOnly = 2 }
public enum ResourcePriority : uint { Regular = 0, Low = 1 }
public enum ResourceUsage : uint { Online = 0, Offline = 1 }
public enum ResourceStoragePolicy : uint { Permanent = 0, Volatile = 1 }
public enum ResourceResponseStatus : uint { Ok = 0, NoContent = 1, NotModified = 2, Error = 3 }
public enum ResourceErrorReason : uint { None = 0, NotFound = 1, Server = 2, Connection = 3, RateLimit = 4, Other = 5 }
public enum ResourceProviderDecision : uint { PassThrough = 0, Handle = 1 }

public readonly record struct ByteRange(ulong Offset, ulong Size);

public sealed record ResourceRequest(
    ResourceKind Kind,
    string Url,
    ResourceLoadingMethod LoadingMethod,
    ResourcePriority Priority,
    ResourceUsage Usage,
    ResourceStoragePolicy StoragePolicy,
    ByteRange? Range,
    DateTimeOffset? PriorModified,
    DateTimeOffset? PriorExpires,
    string? PriorEtag,
    ulong? PriorDataSize);

public sealed record ResourceTransformRequest(ResourceKind Kind, string Url);

public sealed class ResourceResponse
{
    public ResourceResponseStatus Status { get; set; } = ResourceResponseStatus.Ok;
    public ResourceErrorReason ErrorReason { get; set; } = ResourceErrorReason.None;
    public byte[] Bytes { get; set; } = [];
    public string? Etag { get; set; }
    public string? Modified { get; set; }
    public string? Expires { get; set; }
    public bool MustRevalidate { get; set; }
    public bool NoContent { get; set; }
    public bool NotModified { get; set; }
}

public delegate ResourceProviderDecision ResourceProviderCallback(ResourceRequest request, ResourceRequestHandle handle);
public delegate string? ResourceTransformCallback(ResourceTransformRequest request);
