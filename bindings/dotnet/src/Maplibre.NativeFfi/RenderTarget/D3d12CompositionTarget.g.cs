// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A composition swap chain to present to, which your DirectComposition visual
/// or XAML <c>SwapChainPanel</c> shows.
/// </summary>
/// <remarks>
/// <para>The session renders on D3D12: on the D3D12 device that you attach on,
/// or on one that it makes without a device. It makes a flip-model swap chain
/// for composition of <c>logical_size</c> at the map's scale factor, with
/// premultiplied alpha, and sets it as the visual's content or the panel's swap
/// chain. Commit your DirectComposition device once the attach completes. A
/// panel takes its swap chain only on its UI thread: attach it there with
/// <c>RenderDriverKind.CallerGraphicsThread</c>, and service the session there.
/// Scale the visual or the panel to show the swap chain at the display's scale.
/// The session references <c>object</c> until a replacement or release of the
/// target completes, or the session ends.</para>
/// <para>See <c>mln_d3d12_composition_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct D3d12CompositionTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public D3d12CompositionTarget(
        LogicalSize LogicalSize,
        D3d12CompositionKind Kind,
        NativePointer Object
    )
    {
        this.LogicalSize = LogicalSize;
        this.Kind = Kind;
        this.Object = Object;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required D3d12CompositionKind Kind { get; init; }

    /// <summary>
    /// Your <c>IDCompositionVisual*</c> or <c>ISwapChainPanelNative*</c>.
    /// </summary>
    public required NativePointer Object { get; init; }
}
