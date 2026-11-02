// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class HttpHeaderTransformResponse
{
    /// <summary>
    /// Sets a header on the request that an HTTP header transform callback
    /// handles.
    /// </summary>
    /// <remarks>
    /// <para>Header names ignore letter case, and setting a header again
    /// replaces its value. <c>name</c> can't be a header that the library or
    /// the platform manages: <c>Accept-Encoding</c>, <c>Connection</c>,
    /// <c>Content-Length</c>, <c>Host</c>, <c>If-Modified-Since</c>,
    /// <c>If-None-Match</c>, <c>Keep-Alive</c>, <c>Proxy-Authorization</c>,
    /// <c>Proxy-Connection</c>, <c>Range</c>, <c>TE</c>, <c>Trailer</c>,
    /// <c>Transfer-Encoding</c>, <c>Upgrade</c>, or <c>User-Agent</c>.</para>
    /// <para>See <c>mln_http_header_transform_response_set_header</c> in the
    /// <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
    /// </remarks>
    public void SetHeader(string name, string value)
    {
        using var call = Enter(this);
        using var scope = new NativeCallScope();
        Check(
            NativeMethods.mln_http_header_transform_response_set_header(
                Pointer,
                scope.Utf8(name),
                scope.Utf8(value),
                Diagnostic
            )
        );
        scope.Accept();
    }
}
