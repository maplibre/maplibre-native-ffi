// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Frame-demand policy bits.
/// </summary>
/// <remarks>
/// See <c>mln_frame_demand_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum FrameDemandFlag : uint
{
    /// <summary>
    /// Render only when a newer map update exists.
    /// </summary>
    IfNeeded = 1,

    /// <summary>
    /// Present the rendered frame on a target that supports presentation. A
    /// presenting target whose demand clears this bit still renders and keeps
    /// whatever it presented last. Ignored by targets without presentation.
    /// </summary>
    Present = 2,
}
