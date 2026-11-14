// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The frame's Metal command buffer, outside the map's render pass.
/// </summary>
/// <remarks>
/// <para>End each encoder that you make on <c>command_buffer</c> before the
/// callback returns, and don't commit the buffer.</para>
/// <para>See <c>mln_metal_custom_layer_encoder</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="CommandBuffer">
/// The frame's <c>MTLCommandBuffer</c>, with no encoder open.
/// </param>
public readonly partial record struct MetalCustomLayerEncoder(NativePointer CommandBuffer);
