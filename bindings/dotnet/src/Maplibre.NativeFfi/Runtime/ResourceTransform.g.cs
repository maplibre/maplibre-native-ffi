// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Resource transform state.
/// </summary>
/// <remarks>
/// See <c>mln_resource_transform</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct ResourceTransform
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public ResourceTransform(ResourceTransformCallback Callback)
    {
        this.Callback = Callback;
    }

    /// <summary>
    /// Required in callback mode; null in queue mode.
    /// </summary>
    public required ResourceTransformCallback Callback { get; init; }
}
