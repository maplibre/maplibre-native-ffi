// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a frame demand behaves.
/// </summary>
/// <remarks>
/// <para>Without flags, a demand draws a frame when the map changed since the
/// last one, and otherwise finishes with <c>RenderResult.NoUpdate</c>. A
/// session with a surface target shows each frame that it draws.</para>
/// <para>See <c>mln_frame_demand_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.</para>
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
    /// When the map has nothing new to draw, waits for its next update instead
    /// of finishing with <c>RenderResult.NoUpdate</c>.
    /// </summary>
    WaitForUpdate = 4,
}
