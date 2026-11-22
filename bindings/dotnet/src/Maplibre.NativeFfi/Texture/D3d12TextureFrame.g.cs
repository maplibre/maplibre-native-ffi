// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The D3D12 texture of an acquired frame.
/// </summary>
/// <remarks>
/// <para>The depth members are set when the frame has a depth texture, as for
/// <c>MetalTextureFrame</c>. Otherwise they are zero.</para>
/// <para>See <c>mln_d3d12_texture_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Slot">
/// The texture's index in the descriptor's <c>textures</c>, or in the ring.
/// </param>
/// <param name="Resource">
/// The <c>ID3D12Resource</c>.
/// </param>
/// <param name="Device">
/// The resource's <c>ID3D12Device</c>.
/// </param>
/// <param name="Format">
/// The resource's <c>DXGI_FORMAT</c>.
/// </param>
/// <param name="State">
/// The <c>D3D12_RESOURCE_STATES</c> that the frame left the resource in.
/// </param>
/// <param name="DepthResource">
/// The <c>ID3D12Resource</c> that holds the frame's depth and stencil.
/// </param>
/// <param name="DepthFormat">
/// The depth resource's <c>DXGI_FORMAT</c>.
/// </param>
/// <param name="DepthState">
/// The <c>D3D12_RESOURCE_STATES</c> that the frame left the depth resource in.
/// </param>
public readonly partial record struct D3d12TextureFrame(
    uint Width,
    uint Height,
    double ScaleFactor,
    uint Slot,
    NativePointer Resource,
    NativePointer Device,
    uint Format,
    uint State,
    NativePointer DepthResource,
    uint DepthFormat,
    uint DepthState
);
