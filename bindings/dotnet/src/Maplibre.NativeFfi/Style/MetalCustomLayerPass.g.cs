// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The map's Metal render pass, at a custom layer's place in the style.
/// </summary>
/// <remarks>
/// <para>Set your own pipeline state, depth-stencil state, buffers, textures,
/// and samplers on <c>encoder</c>, and any other state that your draws need.
/// After the callback returns, the map sets the state that its own draws need
/// again. Don't end <c>encoder</c>.</para>
/// <para>See <c>mln_metal_custom_layer_pass</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Encoder">
/// The pass's <c>MTLRenderCommandEncoder</c>.
/// </param>
/// <param name="CommandBuffer">
/// The <c>MTLCommandBuffer</c> that <c>encoder</c> encodes into. Don't commit
/// it.
/// </param>
/// <param name="ColorFormat">
/// The <c>MTLPixelFormat</c> of the pass's color attachment.
/// </param>
/// <param name="DepthFormat">
/// The <c>MTLPixelFormat</c> of the pass's depth attachment, or
/// <c>MTLPixelFormatInvalid</c> when it has none.
/// </param>
/// <param name="StencilFormat">
/// The <c>MTLPixelFormat</c> of the pass's stencil attachment, or
/// <c>MTLPixelFormatInvalid</c> when it has none.
/// </param>
/// <param name="SampleCount">
/// The sample count of the pass's attachments.
/// </param>
public readonly partial record struct MetalCustomLayerPass(
    NativePointer Encoder,
    NativePointer CommandBuffer,
    ulong ColorFormat,
    ulong DepthFormat,
    ulong StencilFormat,
    uint SampleCount
);
