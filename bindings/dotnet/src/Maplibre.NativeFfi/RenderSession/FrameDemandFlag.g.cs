// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a frame request behaves.
/// </summary>
/// <remarks>
/// See <c>mln_frame_demand_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum FrameDemandFlag : uint
{
    /// <summary>
    /// Draws a frame even when nothing changed since the last one. It can't be
    /// combined with <c>FrameDemandFlag.WaitForUpdate</c>.
    /// </summary>
    Force = 1,

    /// <summary>
    /// Shows the frame on a surface target.
    /// </summary>
    Present = 2,

    /// <summary>
    /// When the map has nothing new to draw, waits for its next update instead
    /// of finishing with <c>RenderResult.NoUpdate</c>.
    /// </summary>
    WaitForUpdate = 4,
}
