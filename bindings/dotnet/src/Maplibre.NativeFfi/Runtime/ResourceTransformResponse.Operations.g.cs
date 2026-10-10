// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public sealed unsafe partial class ResourceTransformResponse
{
    /// <summary>
    /// Copies a replacement URL into C API-managed storage for the current
    /// callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_resource_transform_response_set_url</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
    /// </remarks>
    public void SetUrl(string url)
    {
        using var call = Enter(this, "mln_resource_transform_response_set_url");
        using var scope = new NativeCallScope();
        var bufferUrl = scope.Utf8(url);
        Check(
            NativeMethods.mln_resource_transform_response_set_url(
                Pointer,
                (sbyte*)bufferUrl.data,
                checked((nuint)bufferUrl.size),
                Diagnostic
            )
        );
        scope.Accept();
    }
}
