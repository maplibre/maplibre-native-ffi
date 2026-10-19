// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One request pattern that a resource provider claims.
/// </summary>
/// <remarks>
/// See <c>mln_resource_route</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Kind">
/// One of <c>mln_resource_kind</c>, or <c>MLN_RESOURCE_KIND_ANY</c>.
/// </param>
/// <param name="Flags">
/// A bitwise OR of <c>mln_url_match_flags</c> values.
/// </param>
/// <param name="Url">
/// The URL or glob pattern to compare.
/// </param>
public readonly partial record struct ResourceRoute(uint Kind, UrlMatchFlags Flags, string Url);
