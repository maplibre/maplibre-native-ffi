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
    /// Native rendering or driver work failed and the session can no longer
    /// render. This does not imply target or device loss. Read the first
    /// failure with <c>RenderSessionHandle.GetFailure()</c>. The map also
    /// reports it as <c>RuntimeEventType.MapRenderError</c>; a failed attach
    /// reports it through its completion instead. Snapshots, frame-result
    /// drains, acquired-frame release, abandon, and dispose remain available.
    /// Dispose this session before attaching a replacement.
    /// </summary>
    Failed = 5,
    Abandoned = 6,
}
