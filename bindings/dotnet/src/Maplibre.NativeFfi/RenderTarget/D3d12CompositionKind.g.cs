// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// What shows a D3D12 composition swap chain.
/// </summary>
/// <remarks>
/// See <c>mln_d3d12_composition_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum D3d12CompositionKind : uint
{
    /// <summary>
    /// An <c>IDCompositionVisual</c>, whose content the session sets to its
    /// swap chain.
    /// </summary>
    Visual = 1,

    /// <summary>
    /// A XAML <c>SwapChainPanel</c>'s <c>ISwapChainPanelNative</c>, of WinUI or
    /// UWP, whose swap chain the session sets.
    /// </summary>
    SwapChainPanel = 2,
}
