// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed unsafe partial class HttpHeaderTransformResponse
{
    private readonly mln_http_header_transform_response* pointer;
    private readonly NativeViewScope scope = new();

    internal HttpHeaderTransformResponse(mln_http_header_transform_response* pointer) =>
        this.pointer = pointer;

    internal mln_http_header_transform_response* Pointer
    {
        get
        {
            scope.EnsureActive();
            return pointer;
        }
    }

    internal void Expire() => scope.Expire();
}
