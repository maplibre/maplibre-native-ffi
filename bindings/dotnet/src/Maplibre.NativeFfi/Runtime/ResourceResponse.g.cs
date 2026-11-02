// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The answer to a resource request.
/// </summary>
/// <remarks>
/// See <c>mln_resource_response</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public sealed record ResourceResponse
{
    public ResourceResponseStatus Status { get; init; }

    /// <summary>
    /// Why the request failed, for <c>ResourceResponseStatus.Error</c>.
    /// </summary>
    public ResourceErrorReason ErrorReason { get; init; }

    /// <summary>
    /// The resource, for <c>ResourceResponseStatus.Ok</c>.
    /// </summary>
    public ImmutableArray<byte> Bytes
    {
        get => BytesStorage.AsImmutable();
        init => BytesStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> BytesStorage { get; init; }

    /// <summary>
    /// A description of the error, for <c>ResourceResponseStatus.Error</c>.
    /// </summary>
    public string ErrorMessage { get; init; } = "";

    /// <summary>
    /// Whether the library must request the resource again before it reuses
    /// this copy.
    /// </summary>
    public bool MustRevalidate { get; init; }
    public long? ModifiedUnixMs { get; init; }
    public long? ExpiresUnixMs { get; init; }

    /// <summary>
    /// Empty when the resource has no ETag.
    /// </summary>
    public string Etag { get; init; } = "";

    /// <summary>
    /// When to retry, for <c>ResourceResponseStatus.Error</c>.
    /// </summary>
    public long? RetryAfterUnixMs { get; init; }
}
