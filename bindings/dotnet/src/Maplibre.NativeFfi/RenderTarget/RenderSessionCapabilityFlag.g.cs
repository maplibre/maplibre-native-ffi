// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Optional render-session capabilities.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_capability_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum RenderSessionCapabilityFlag : uint
{
    FrameAcquisition = 1,
    Readback = 2,
    ConsumerSync = 4,
    Presentation = 8,
}
