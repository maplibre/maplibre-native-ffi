// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One HTTP header that a header rule supplies.
/// </summary>
/// <remarks>
/// See <c>mln_http_header</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct HttpHeader(string Name, string Value);
