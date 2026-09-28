// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Runtime;

public sealed record ResourceResponse
{
    public ResourceResponseStatus Status { get; set; }
    public ResourceErrorReason ErrorReason { get; set; }
    private byte[]? storageBytes;
    public byte[] Bytes
    {
        get => storageBytes?.ToArray() ?? [];
        set => storageBytes = value?.ToArray() ?? [];
    }
    internal byte[] BytesStorage
    {
        get => storageBytes ?? [];
        init => storageBytes = value;
    }
    public string? ErrorMessage { get; set; }
    public bool MustRevalidate { get; set; }
    public long? ModifiedUnixMs { get; set; }
    public long? ExpiresUnixMs { get; set; }
    public string? Etag { get; set; }
    public long? RetryAfterUnixMs { get; set; }

    public bool Equals(ResourceResponse? other) =>
        other is not null
        && EqualityComparer<ResourceResponseStatus>.Default.Equals(Status, other.Status)
        && EqualityComparer<ResourceErrorReason>.Default.Equals(ErrorReason, other.ErrorReason)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            BytesStorage,
            other.BytesStorage
        )
        && EqualityComparer<string?>.Default.Equals(ErrorMessage, other.ErrorMessage)
        && EqualityComparer<bool>.Default.Equals(MustRevalidate, other.MustRevalidate)
        && EqualityComparer<long?>.Default.Equals(ModifiedUnixMs, other.ModifiedUnixMs)
        && EqualityComparer<long?>.Default.Equals(ExpiresUnixMs, other.ExpiresUnixMs)
        && EqualityComparer<string?>.Default.Equals(Etag, other.Etag)
        && EqualityComparer<long?>.Default.Equals(RetryAfterUnixMs, other.RetryAfterUnixMs);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Status);
        hash.Add(ErrorReason);
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(BytesStorage));
        hash.Add(ErrorMessage);
        hash.Add(MustRevalidate);
        hash.Add(ModifiedUnixMs);
        hash.Add(ExpiresUnixMs);
        hash.Add(Etag);
        hash.Add(RetryAfterUnixMs);
        return hash.ToHashCode();
    }
}
