// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public sealed unsafe partial class ResourceTransformResponse
{
    public void SetUrl(string url)
    {
        using var scope = new NativeCallScope(this, "mln_resource_transform_response_set_url");
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
