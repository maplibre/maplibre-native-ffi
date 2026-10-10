// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a resource provider answered a request.
/// </summary>
/// <remarks>
/// See <c>mln_resource_response_status</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum ResourceResponseStatus : uint
{
    Ok = 0,
    Error = 1,
    NoContent = 2,
    NotModified = 3,
}
