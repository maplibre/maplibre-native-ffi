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

    /// <summary>
    /// With <c>MLN_FRAME_DEMAND_IF_NEEDED</c>, a demand that would finish with
    /// <c>MLN_RENDER_RESULT_NO_UPDATE</c> or
    /// <c>MLN_RENDER_RESULT_SIZE_PENDING</c> waits instead, and runs again
    /// after the map's next update, a target replacement, or an applied resize.
    /// A waiting demand holds no ring slot. A later demand with the same flags
    /// and coalescing boundary supersedes it, a barrier ends its wait with
    /// <c>MLN_RENDER_RESULT_NO_UPDATE</c>, and detach, abandon, or the
    /// quarantine of the ring's last usable slot end it with
    /// <c>MLN_RENDER_RESULT_TARGET_NOT_READY</c>. A waiting demand's result can
    /// follow the results of demands accepted after it. The flag does not pace:
    /// a host that re-arms a waiting demand as each result arrives renders
    /// every update the map publishes.
    /// </summary>
    WaitForUpdate = 4,
}
