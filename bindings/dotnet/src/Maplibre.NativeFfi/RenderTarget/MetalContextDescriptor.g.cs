// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Metal backend context fields shared by Metal render targets.
/// </summary>
/// <remarks>
/// See <c>mln_metal_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Device">
/// <c>id&lt;MTLDevice&gt;</c> / <c>MTL::Device*</c>. Retained when the target
/// requires it.
/// </param>
public readonly partial record struct MetalContextDescriptor(NativePointer Device);
