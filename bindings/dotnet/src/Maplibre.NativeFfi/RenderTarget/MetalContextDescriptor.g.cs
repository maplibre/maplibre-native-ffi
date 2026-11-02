// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The Metal device that a render session uses.
/// </summary>
/// <remarks>
/// See <c>mln_metal_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Device">
/// An <c>MTLDevice</c>.
/// </param>
public readonly partial record struct MetalContextDescriptor(NativePointer Device);
