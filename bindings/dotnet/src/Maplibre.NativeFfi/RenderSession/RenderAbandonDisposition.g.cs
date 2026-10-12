// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// What abandon did with a session's graphics resources.
/// </summary>
/// <remarks>
/// See <c>mln_render_abandon_disposition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public enum RenderAbandonDisposition : uint
{
    /// <summary>
    /// Abandon destroyed every graphics resource, or none remained.
    /// </summary>
    Clean = 0,

    /// <summary>
    /// Abandon kept graphics resources that it could not safely destroy.
    /// </summary>
    Quarantined = 1,
}
