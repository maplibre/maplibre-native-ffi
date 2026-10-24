// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Render-session lifecycle visible in snapshots.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_state</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public enum RenderSessionState : uint
{
    Attaching = 1,
    Attached = 2,
    Detaching = 3,
    Detached = 4,

    /// <summary>
    /// A frame or driver task failed and the session can no longer render. The
    /// map reports the failure as <c>MLN_RUNTIME_EVENT_MAP_RENDER_ERROR</c>. A
    /// failed attach reports it through its completion instead.
    /// </summary>
    TargetLost = 5,
    Abandoned = 6,
}
