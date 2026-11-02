// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A cancel callback, for <c>ResourceRequestHandle.SetCancelCallback()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_resource_request_cancel_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ResourceRequestCancelHandler
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public ResourceRequestCancelHandler(ResourceRequestCancelCallback Callback)
    {
        this.Callback = Callback;
    }

    public required ResourceRequestCancelCallback Callback { get; init; }
}
