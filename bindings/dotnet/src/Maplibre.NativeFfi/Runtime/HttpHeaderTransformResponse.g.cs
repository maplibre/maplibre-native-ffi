// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed unsafe partial class HttpHeaderTransformResponse
{
    private readonly global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackScope<mln_http_header_transform_response> scope;

    internal HttpHeaderTransformResponse(mln_http_header_transform_response* pointer) =>
        scope = new(pointer);

    internal mln_http_header_transform_response* Pointer => scope.Pointer;

    internal void Expire() => scope.Expire();
}
