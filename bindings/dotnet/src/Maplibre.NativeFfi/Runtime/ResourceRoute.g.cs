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
/// <param name="Kinds">
/// The resource kinds to match. Zero selects every kind.
/// </param>
/// <param name="Url">
/// The URL or glob pattern to compare.
/// </param>
public readonly partial record struct ResourceRoute(
    ResourceKindFlag Kinds,
    UrlMatchFlag Flags,
    string Url
)
{
    public ResourceRoute()
        : this(default, default, "") { }
}
