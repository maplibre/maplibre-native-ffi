// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A format of depth and stencil textures, as a bit in a set.
/// </summary>
/// <remarks>
/// See <c>mln_depth_stencil_format_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum DepthStencilFormatFlag : uint
{
    /// <summary>
    /// 24-bit normalized depth and 8-bit stencil: VK_FORMAT_D24_UNORM_S8_UINT,
    /// MTLPixelFormatDepth24Unorm_Stencil8, GL_DEPTH24_STENCIL8, or
    /// WGPUTextureFormat_Depth24PlusStencil8 (<c>depth24plus-stencil8</c>).
    /// </summary>
    Depth24Stencil8 = 1,

    /// <summary>
    /// 32-bit float depth and 8-bit stencil: VK_FORMAT_D32_SFLOAT_S8_UINT,
    /// MTLPixelFormatDepth32Float_Stencil8, GL_DEPTH32F_STENCIL8, or
    /// WGPUTextureFormat_Depth32FloatStencil8 (<c>depth32float-stencil8</c>).
    /// </summary>
    Depth32FloatStencil8 = 2,
}
