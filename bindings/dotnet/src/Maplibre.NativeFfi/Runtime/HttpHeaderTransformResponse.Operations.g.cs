// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class HttpHeaderTransformResponse
{
    /// <summary>
    /// Sets one outgoing HTTP request header for the current transform
    /// invocation.
    /// </summary>
    /// <remarks>
    /// See <c>mln_http_header_transform_response_set_header</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
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
