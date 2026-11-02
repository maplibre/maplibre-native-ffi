// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The requests that a resource provider serves.
/// </summary>
/// <remarks>
/// See <c>mln_resource_route</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Kinds">
/// The kinds of resource that the route covers. Zero covers every kind.
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
