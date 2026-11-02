// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Something that a render session can do.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_capability_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum RenderSessionCapabilityFlag : uint
{
    /// <summary>
    /// <c>RenderSessionHandle.AcquireFrame()</c> can take the frames it
    /// renders.
    /// </summary>
    FrameAcquisition = 1,

    /// <summary>
    /// <c>RenderSessionHandle.ReadTextureAsync()</c> can copy its texture to
    /// memory.
    /// </summary>
    Readback = 2,

    /// <summary>
    /// It shows frames on a surface.
    /// </summary>
    Presentation = 4,
}
