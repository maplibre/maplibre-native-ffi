// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of DXGI shared textures that the library allocates, for
/// <c>MapHandle.AttachDxgiSharedTextureRing()</c>.
/// </summary>
/// <remarks>
/// <para>Each texture is a D3D11 2D texture with the map's size in pixels and
/// the format DXGI_FORMAT_B8G8R8A8_UNORM. It binds as a shader resource and a
/// render target, and is shared through an NT handle.</para>
/// <para>See <c>mln_dxgi_shared_texture_ring_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/win32_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct DxgiSharedTextureRingDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public DxgiSharedTextureRingDescriptor(LogicalSize LogicalSize, uint TextureCount)
    {
        this.LogicalSize = LogicalSize;
        this.TextureCount = TextureCount;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// How many textures the ring has, from 1 to 8.
    /// </summary>
    public required uint TextureCount { get; init; }
}
