namespace Maplibre.Native.Resource;

public enum ResourceKind : uint
{
    Unknown = 0,
    Style = 1,
    Source = 2,
    Tile = 3,
    Glyphs = 4,
    SpriteImage = 5,
    SpriteJson = 6,
    Image = 7,
}

public enum ResourceLoadingMethod : uint
{
    All = 0,
    CacheOnly = 1,
    NetworkOnly = 2,
}

public enum ResourcePriority : uint
{
    Regular = 0,
    Low = 1,
}

public enum ResourceUsage : uint
{
    Online = 0,
    Offline = 1,
}

public enum ResourceStoragePolicy : uint
{
    Permanent = 0,
    Volatile = 1,
}

public enum ResourceResponseStatus : uint
{
    Ok = 0,
    Error = 1,
    NoContent = 2,
    NotModified = 3,
}

public enum ResourceErrorReason : uint
{
    None = 0,
    NotFound = 1,
    Server = 2,
    Connection = 3,
    RateLimit = 4,
    Other = 5,
}

public enum ResourceProviderDecision : uint
{
    PassThrough = 0,
    Handle = 1,
}

public readonly record struct ByteRange(ulong Start, ulong End);

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
    ulong? PriorDataSize,
    byte[]? PriorData
);

public sealed record ResourceTransformRequest(ResourceKind Kind, string Url);

/// <summary>Mutable descriptor used to complete a resource provider request.</summary>
public sealed class ResourceResponse
{
    private byte[] bytes = [];

    public ResourceResponse(ResourceResponseStatus status = ResourceResponseStatus.Ok)
    {
        Status = status;
    }

    public static ResourceResponse Ok(ReadOnlySpan<byte> bytes) =>
        new(ResourceResponseStatus.Ok) { Bytes = bytes.ToArray() };

    public static ResourceResponse NoContent() => new(ResourceResponseStatus.NoContent);

    public static ResourceResponse NotModified() => new(ResourceResponseStatus.NotModified);

    public static ResourceResponse Error(ResourceErrorReason reason, string? message) =>
        new(ResourceResponseStatus.Error) { ErrorReason = reason, ErrorMessage = message };

    public ResourceResponseStatus Status { get; set; }
    public ResourceErrorReason ErrorReason { get; set; } = ResourceErrorReason.None;

    public byte[] Bytes
    {
        get => (byte[])bytes.Clone();
        set => bytes = value is null ? [] : (byte[])value.Clone();
    }

    public string? ErrorMessage { get; set; }
    public bool MustRevalidate { get; set; }
    public DateTimeOffset? Modified { get; set; }
    public DateTimeOffset? Expires { get; set; }
    public string? Etag { get; set; }
    public DateTimeOffset? RetryAfter { get; set; }
}

public delegate ResourceProviderDecision ResourceProviderCallback(
    ResourceRequest request,
    ResourceRequestHandle handle
);
public delegate string? ResourceTransformCallback(ResourceTransformRequest request);
