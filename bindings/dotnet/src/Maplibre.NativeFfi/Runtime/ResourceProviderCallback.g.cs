// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Intercepts a network resource request. The callback runs synchronously on a
/// MapLibre thread.
/// </summary>
/// <remarks>
/// See <c>mln_resource_provider_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public delegate void ResourceProviderCallback(
    ResourceRequestInfo request,
    ResourceRequestHandle handle
);
