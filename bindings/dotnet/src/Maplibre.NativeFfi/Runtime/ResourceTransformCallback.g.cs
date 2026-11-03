// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Rewrites the URL of a network request before the library sends it.
/// </summary>
/// <remarks>
/// <para>The request waits until you answer with
/// <c>ResourceTransformRequestHandle.Complete()</c> or
/// <c>ResourceTransformRequestHandle.Fail()</c>. To keep <c>url</c>, complete
/// with it. Releasing <c>request</c> without an answer fails the request. You
/// can answer now or later, from any thread.</para>
/// <para>See <c>mln_resource_transform_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void ResourceTransformCallback(
    ResourceKind kind,
    string url,
    ResourceTransformRequestHandle request
);
