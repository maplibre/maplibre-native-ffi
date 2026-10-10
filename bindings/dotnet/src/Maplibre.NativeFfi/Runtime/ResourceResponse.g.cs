// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record ResourceResponse
{
    public ResourceResponseStatus Status { get; set; }
    public ResourceErrorReason ErrorReason { get; set; }
    public byte[] Bytes
    {
        get => BytesStorage.ToArray();
        set => BytesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> BytesStorage { get; set; }
    public string? ErrorMessage { get; set; }
    public bool MustRevalidate { get; set; }
    public long? ModifiedUnixMs { get; set; }
    public long? ExpiresUnixMs { get; set; }
    public string? Etag { get; set; }
    public long? RetryAfterUnixMs { get; set; }
}
