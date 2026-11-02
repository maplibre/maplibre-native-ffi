// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Whether the library keeps a loaded resource in its cache.
/// </summary>
/// <remarks>
/// See <c>mln_resource_storage_policy</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum ResourceStoragePolicy : uint
{
    Permanent = 0,
    Volatile = 1,
}
