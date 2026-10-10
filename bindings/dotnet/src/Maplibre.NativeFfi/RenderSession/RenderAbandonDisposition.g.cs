// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Result of irreversible CPU-side target abandonment.
/// </summary>
/// <remarks>
/// See <c>mln_render_abandon_disposition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
public enum RenderAbandonDisposition : uint
{
    /// <summary>
    /// No graphics resources remained when control was abandoned.
    /// </summary>
    Clean = 0,

    /// <summary>
    /// Graphics resources could not be destroyed and were quarantined.
    /// </summary>
    Quarantined = 1,
}
