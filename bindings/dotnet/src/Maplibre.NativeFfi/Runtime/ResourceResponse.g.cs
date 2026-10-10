// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A resource provider's answer to one request.
/// </summary>
/// <remarks>
/// See <c>mln_resource_response</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
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
