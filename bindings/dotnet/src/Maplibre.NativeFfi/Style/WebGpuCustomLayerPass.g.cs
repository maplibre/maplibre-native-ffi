// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The map's WebGPU render pass, at a custom layer's place in the style.
/// </summary>
/// <remarks>
/// <para>Set your own pipeline, bind groups, vertex and index buffers,
/// viewport, scissor rectangle, and stencil reference on <c>encoder</c>. After
/// the callback returns, the map sets again what its own draws need. Don't end
/// <c>encoder</c>.</para>
/// <para>See <c>mln_web_gpu_custom_layer_pass</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Device">
/// The <c>GPUDevice</c> that the session renders on.
/// </param>
/// <param name="Encoder">
/// The pass's <c>GPURenderPassEncoder</c>.
/// </param>
/// <param name="ColorFormat">
/// The <c>GPUTextureFormat</c> of the pass's color attachment.
/// </param>
/// <param name="DepthStencilFormat">
/// The <c>GPUTextureFormat</c> of the pass's depth-stencil attachment, or empty
/// when it has none.
/// </param>
/// <param name="SampleCount">
/// The sample count of the pass's attachments.
/// </param>
public readonly partial record struct WebGpuCustomLayerPass(
    uint Device,
    uint Encoder,
    string ColorFormat,
    string DepthStencilFormat,
    uint SampleCount
);
