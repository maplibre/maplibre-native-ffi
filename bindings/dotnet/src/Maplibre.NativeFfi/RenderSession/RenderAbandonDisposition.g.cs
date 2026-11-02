// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// What <c>RenderSessionHandle.Abandon()</c> did with a session's graphics
/// objects.
/// </summary>
/// <remarks>
/// See <c>mln_render_abandon_disposition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public enum RenderAbandonDisposition : uint
{
    Clean = 0,

    /// <summary>
    /// It kept some objects that it couldn't destroy safely. They stay
    /// allocated until the process exits.
    /// </summary>
    Quarantined = 1,
}
