// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a request may load its resource from.
/// </summary>
/// <remarks>
/// See <c>mln_resource_loading_method</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum ResourceLoadingMethod : uint
{
    /// <summary>
    /// The cache or the network.
    /// </summary>
    All = 0,
    CacheOnly = 1,
    NetworkOnly = 2,
}
