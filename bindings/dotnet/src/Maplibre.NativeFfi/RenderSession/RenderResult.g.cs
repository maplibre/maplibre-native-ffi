// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a frame demand finished.
/// </summary>
/// <remarks>
/// See <c>mln_render_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public enum RenderResult : uint
{
    Rendered = 0,

    /// <summary>
    /// The map had nothing new to draw.
    /// </summary>
    NoUpdate = 1,

    /// <summary>
    /// The target couldn't take a frame, such as while a window is hidden or
    /// after <c>RenderSessionHandle.ReleaseTargetAsync()</c>. Demand a frame
    /// again later.
    /// </summary>
    TargetNotReady = 2,

    /// <summary>
    /// A later demand with the same flags replaced this one.
    /// </summary>
    Superseded = 3,

    /// <summary>
    /// The session failed, and <c>RenderSessionHandle.GetFailure()</c> reports
    /// why. Once <c>RenderSessionHandle.CloseAsync()</c> completes, you can
    /// attach a new session.
    /// </summary>
    Failed = 4,
}
