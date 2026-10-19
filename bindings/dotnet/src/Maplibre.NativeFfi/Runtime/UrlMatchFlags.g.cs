// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a rule or route compares its url against a request URL.
/// </summary>
/// <remarks>
/// See <c>mln_url_match_flags</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum UrlMatchFlags : uint
{
    Exact = 0,

    /// <summary>
    /// Reads url as a glob pattern.
    /// </summary>
    Glob = 1,
}
