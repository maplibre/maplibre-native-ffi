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
/// <param name="Callback">
/// Required in callback mode; null in queue mode.
/// </param>
public readonly partial record struct ResourceTransform(
    Action<ResourceKind, string, ResourceTransformRequestHandle>? Callback
);
