// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public sealed unsafe partial class HttpHeaderTransformResponse
{
    public void Set(string name, string value)
    {
        using var scope = new NativeCallScope(this, "mln_http_header_transform_response_set");
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
