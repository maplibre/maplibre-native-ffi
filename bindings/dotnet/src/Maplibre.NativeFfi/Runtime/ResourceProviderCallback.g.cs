// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Serves a network request in place of the library.
/// </summary>
/// <remarks>
/// <para>Answer with <c>ResourceRequestHandle.Complete()</c>, or hand the
/// request back to the library with <c>ResourceRequestHandle.PassThrough()</c>.
/// Either can come now or later, from any thread. A request released without an
/// answer fails.</para>
/// <para>See <c>mln_resource_provider_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void ResourceProviderCallback(
    ResourceRequestInfo request,
    ResourceRequestHandle handle
);
