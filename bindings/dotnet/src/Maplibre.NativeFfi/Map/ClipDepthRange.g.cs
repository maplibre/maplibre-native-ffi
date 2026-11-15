// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a graphics API's clip space holds depth.
/// </summary>
/// <remarks>
/// See <c>mln_clip_depth_range</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum ClipDepthRange : uint
{
    /// <summary>
    /// Clip z runs from 0 to w, as in Metal, Vulkan, WebGPU, Direct3D, and
    /// OpenGL with <c>glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)</c>.
    /// </summary>
    ZeroToOne = 1,

    /// <summary>
    /// Clip z runs from -w to w, as in OpenGL by default.
    /// </summary>
    NegativeOneToOne = 2,
}
