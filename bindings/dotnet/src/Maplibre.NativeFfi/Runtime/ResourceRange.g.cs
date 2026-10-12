// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Inclusive byte range of a resource request.
/// </summary>
/// <remarks>
/// See <c>mln_resource_range</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Start">
/// First byte offset of the requested range.
/// </param>
/// <param name="End">
/// Last byte offset of the requested range, inclusive.
/// </param>
public readonly partial record struct ResourceRange(ulong Start, ulong End);
