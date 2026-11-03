// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Why a resource transform fails a request.
/// </summary>
/// <remarks>
/// See <c>mln_resource_transform_error</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public sealed record ResourceTransformError
{
    public ResourceErrorReason Reason { get; init; }
    public string Message { get; init; } = "";

    /// <summary>
    /// When to retry, for <c>ResourceErrorReason.RateLimit</c>.
    /// </summary>
    public long? RetryAfterUnixMs { get; init; }
}
