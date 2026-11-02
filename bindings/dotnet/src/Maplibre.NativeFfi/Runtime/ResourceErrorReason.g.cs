// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Why a resource request failed.
/// </summary>
/// <remarks>
/// See <c>mln_resource_error_reason</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum ResourceErrorReason : uint
{
    None = 0,
    NotFound = 1,

    /// <summary>
    /// The server reported an error.
    /// </summary>
    Server = 2,

    /// <summary>
    /// The server couldn't be reached.
    /// </summary>
    Connection = 3,
    RateLimit = 4,
    Other = 5,
}
