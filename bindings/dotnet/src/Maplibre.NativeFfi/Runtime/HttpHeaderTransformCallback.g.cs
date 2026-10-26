// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Adds end-to-end headers to one outgoing HTTP request attempt. The callback
/// runs synchronously on a MapLibre worker or network thread.
/// </summary>
/// <remarks>
/// <para>Returning succeeds. Throwing a <c>MaplibreException</c> whose status
/// is not <c>Status.Ok</c> fails the call with that status. Any other exception
/// fails it with <c>Status.NativeError</c> and is reported to
/// <c>MaplibreNative.CallbackException</c>.</para>
/// <para>See <c>mln_http_header_transform_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void HttpHeaderTransformCallback(
    ResourceKind kind,
    string url,
    HttpHeaderTransformResponse outResponse
);
