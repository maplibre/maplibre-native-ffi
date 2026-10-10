// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_resource_request</c>.
/// </summary>
/// <remarks>
/// See <c>mln_resource_request_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum ResourceRequestField : uint
{
    /// <summary>
    /// The request asks only for the bytes in range.
    /// </summary>
    Range = 1,

    /// <summary>
    /// The cached copy being revalidated carries a modification time.
    /// </summary>
    PriorModified = 2,

    /// <summary>
    /// The cached copy being revalidated carries an expiration time.
    /// </summary>
    PriorExpires = 4,
}
