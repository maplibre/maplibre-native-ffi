// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The bytes from <c>start</c> to <c>end</c> of a resource, both included.
/// </summary>
/// <remarks>
/// See <c>mln_resource_range</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ResourceRange(ulong Start, ulong End);
