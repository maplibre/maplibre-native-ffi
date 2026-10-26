// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Reports that MapLibre cancelled a C API resource provider request. The
/// callback runs on the thread that discards the request, never a host thread:
/// the runtime's native scheduler thread when a committed map or runtime
/// command discards it, such as a style change or map destruction, and a
/// MapLibre worker thread otherwise.
/// </summary>
/// <remarks>
/// See <c>mln_resource_request_cancel_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public delegate void ResourceRequestCancelCallback();
