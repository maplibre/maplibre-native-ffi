// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The map's D3D12 render pass, at a custom layer's place in the style.
/// </summary>
/// <remarks>
/// <para>Set your own pipeline state, root signature, descriptor heaps, vertex
/// and index buffers, primitive topology, viewport, scissor rectangle, and
/// stencil reference, with pipeline states for <c>color_format</c> and
/// <c>depth_stencil_format</c>, and draw. After the callback returns, the map
/// sets again what its own draws need. Record nothing that a render pass can't
/// hold, such as barriers, copies, or other render targets, and don't end the
/// pass.</para>
/// <para>See <c>mln_d3d12_custom_layer_pass</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Device">
/// The <c>ID3D12Device</c> that the session renders on, to make objects with.
/// </param>
/// <param name="CommandList">
/// The frame's <c>ID3D12GraphicsCommandList</c>, in the render pass that
/// <c>BeginRenderPass</c> began.
/// </param>
/// <param name="RenderTargetView">
/// The <c>D3D12_CPU_DESCRIPTOR_HANDLE</c> of the pass's render target view.
/// </param>
/// <param name="ColorFormat">
/// The <c>DXGI_FORMAT</c> of the pass's color attachment.
/// </param>
/// <param name="DepthStencilView">
/// The <c>D3D12_CPU_DESCRIPTOR_HANDLE</c> of the pass's depth-stencil view, or
/// 0 when it has none.
/// </param>
/// <param name="DepthStencilFormat">
/// The <c>DXGI_FORMAT</c> of the pass's depth-stencil attachment, or
/// <c>DXGI_FORMAT_UNKNOWN</c> when it has none.
/// </param>
public readonly partial record struct D3d12CustomLayerPass(
    NativePointer Device,
    NativePointer CommandList,
    ulong RenderTargetView,
    uint ColorFormat,
    ulong DepthStencilView,
    uint DepthStencilFormat
);
