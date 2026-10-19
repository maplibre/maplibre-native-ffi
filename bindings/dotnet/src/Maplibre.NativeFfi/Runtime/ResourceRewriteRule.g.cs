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
/// <param name="Kinds">
/// The resource kinds to match.
/// </param>
/// <param name="Flags">
/// A bitwise OR of <c>mln_url_match_flags</c> values.
/// </param>
/// <param name="Url">
/// The URL or glob pattern to compare.
/// </param>
/// <param name="ReplacementUrl">
/// The replacement URL, or empty to keep the URL and stop the search.
/// </param>
public readonly partial record struct ResourceRewriteRule(
    ResourceKindMask Kinds,
    UrlMatchFlags Flags,
    string Url,
    string? ReplacementUrl
)
{
    public ResourceRewriteRule()
        : this(ResourceKindMask.All, default, default!, default) { }
}
