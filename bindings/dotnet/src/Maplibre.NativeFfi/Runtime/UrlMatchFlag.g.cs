// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a URL pattern matches a request's URL.
/// </summary>
/// <remarks>
/// <para>A pattern matches the whole URL, query included. Without flags, it
/// matches only the URL that equals it exactly.</para>
/// <para>See <c>mln_url_match_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
[Flags]
public enum UrlMatchFlag : uint
{
    /// <summary>
    /// The URL matches the pattern as a glob. <c>*</c> matches any characters
    /// except <c>/</c>, <c>**</c> matches any characters, and <c>?</c> matches
    /// one character except <c>/</c>. <c>\</c> makes the next character match
    /// only itself. For example, <c>https://*.example.com/tiles/**</c> matches
    /// every URL under <c>/tiles/</c> on each subdomain of <c>example.com</c>.
    /// </summary>
    Glob = 1,
}
