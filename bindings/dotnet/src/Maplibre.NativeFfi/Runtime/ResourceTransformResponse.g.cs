// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Runtime;

public sealed unsafe partial class ResourceTransformResponse
{
    private readonly global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackScope<mln_resource_transform_response> scope;

    internal ResourceTransformResponse(mln_resource_transform_response* pointer) =>
        scope = new(pointer);

    internal mln_resource_transform_response* Pointer => scope.Pointer;

    internal void Expire() => scope.Expire();
}
