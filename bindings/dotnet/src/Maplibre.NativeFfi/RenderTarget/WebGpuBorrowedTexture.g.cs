// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A WebGPU texture of a borrowed ring.
/// </summary>
/// <remarks>
/// See <c>mln_web_gpu_borrowed_texture</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct WebGpuBorrowedTexture
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public WebGpuBorrowedTexture(uint Texture)
    {
        this.Texture = Texture;
    }

    /// <summary>
    /// Your <c>GPUTexture</c>.
    /// </summary>
    public required uint Texture { get; init; }
}
