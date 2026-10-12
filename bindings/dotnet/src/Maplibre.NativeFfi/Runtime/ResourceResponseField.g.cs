// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_resource_response</c>.
/// </summary>
/// <remarks>
/// See <c>mln_resource_response_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum ResourceResponseField : uint
{
    /// <summary>
    /// The response carries a modification time.
    /// </summary>
    Modified = 1,

    /// <summary>
    /// The response carries an expiration time.
    /// </summary>
    Expires = 2,

    /// <summary>
    /// An ERROR response carries the earliest time to retry the request.
    /// </summary>
    RetryAfter = 4,
}
