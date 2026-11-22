// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The frame's D3D12 command list, outside the map's render pass.
/// </summary>
/// <remarks>
/// <para>Record work such as copies and render passes of your own, with the
/// barriers that it needs, and leave the resources that you use in the states
/// that you found them in. Don't close or execute <c>command_list</c>. After
/// the callback returns, the map sets its own descriptor heaps and root
/// signature again.</para>
/// <para>See <c>mln_d3d12_custom_layer_encoder</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Device">
/// The <c>ID3D12Device</c> that the session renders on, to make objects with.
/// </param>
/// <param name="CommandList">
/// The frame's <c>ID3D12GraphicsCommandList</c>, outside a render pass.
/// </param>
public readonly partial record struct D3d12CustomLayerEncoder(
    NativePointer Device,
    NativePointer CommandList
);
