// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A D3D12 texture of a borrowed ring.
/// </summary>
/// <remarks>
/// See <c>mln_d3d12_borrowed_texture</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct D3d12BorrowedTexture
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public D3d12BorrowedTexture(NativePointer Resource)
    {
        this.Resource = Resource;
    }

    /// <summary>
    /// Your <c>ID3D12Resource</c>.
    /// </summary>
    public required NativePointer Resource { get; init; }
}
