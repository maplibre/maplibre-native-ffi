// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The frame's WebGPU command encoder, outside the map's render pass.
/// </summary>
/// <remarks>
/// <para>End each pass that you begin on <c>encoder</c> before the callback
/// returns, and don't finish the encoder.</para>
/// <para>See <c>mln_web_gpu_custom_layer_encoder</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Device">
/// The <c>GPUDevice</c> that the session renders on.
/// </param>
/// <param name="Encoder">
/// The frame's <c>GPUCommandEncoder</c>, with no pass open.
/// </param>
public readonly partial record struct WebGpuCustomLayerEncoder(uint Device, uint Encoder);
