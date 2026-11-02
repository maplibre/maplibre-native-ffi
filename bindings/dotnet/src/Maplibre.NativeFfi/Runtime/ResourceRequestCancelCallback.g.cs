// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Reports that the library no longer needs a resource request.
/// </summary>
/// <remarks>
/// <para>The request no longer accepts an answer, so stop working on it and
/// release it.</para>
/// <para>See <c>mln_resource_request_cancel_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void ResourceRequestCancelCallback();
