// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The outcome of a resource request.
/// </summary>
/// <remarks>
/// See <c>mln_resource_response_status</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum ResourceResponseStatus : uint
{
    Ok = 0,
    Error = 1,

    /// <summary>
    /// The resource exists and is empty.
    /// </summary>
    NoContent = 2,

    /// <summary>
    /// The earlier copy that the request's <c>prior_</c> fields describe is
    /// current.
    /// </summary>
    NotModified = 3,
}
