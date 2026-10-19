// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One rule of a native resource transform.
/// </summary>
/// <remarks>
/// See <c>mln_resource_rewrite_rule</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Kind">
/// One of <c>mln_resource_kind</c>, or <c>MLN_RESOURCE_KIND_ANY</c>.
/// </param>
/// <param name="Flags">
/// A bitwise OR of <c>mln_url_match_flags</c> values other than REQUESTED_URL.
/// </param>
/// <param name="Url">
/// The URL or glob pattern to compare.
/// </param>
/// <param name="ReplacementUrl">
/// The replacement URL, or null to leave a matching URL unchanged.
/// </param>
public readonly partial record struct ResourceRewriteRule(
    uint Kind,
    UrlMatchFlags Flags,
    string Url,
    string? ReplacementUrl
);
