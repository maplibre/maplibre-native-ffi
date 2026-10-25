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
    public ResourceResponseStatus Status { get; init; }
    public ResourceErrorReason ErrorReason { get; init; }

    /// <summary>
    /// Response bytes. May be null only when byte_count is 0.
    /// </summary>
    public ImmutableArray<byte> Bytes
    {
        get => BytesStorage.AsImmutable();
        init => BytesStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> BytesStorage { get; init; }
    public string ErrorMessage { get; init; } = "";
    public bool MustRevalidate { get; init; }
    public long? ModifiedUnixMs { get; init; }
    public long? ExpiresUnixMs { get; init; }
    public string Etag { get; init; } = "";
    public long? RetryAfterUnixMs { get; init; }
}
