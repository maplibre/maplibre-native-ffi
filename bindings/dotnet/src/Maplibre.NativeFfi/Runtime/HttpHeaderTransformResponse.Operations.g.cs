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
    /// See <c>mln_http_header_transform_response_set</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void Set(string name, string value)
    {
        using var call = Enter(this, "mln_http_header_transform_response_set");
        using var scope = new NativeCallScope();
        var bufferName = scope.Utf8(name);
        var bufferValue = scope.Utf8(value);
        Check(
            NativeMethods.mln_http_header_transform_response_set(
                Pointer,
                (sbyte*)bufferName.data,
                checked((nuint)bufferName.size),
                (sbyte*)bufferValue.data,
                checked((nuint)bufferValue.size),
                Diagnostic
            )
        );
        scope.Accept();
    }
}
