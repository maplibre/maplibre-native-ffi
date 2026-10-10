// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Execution placement for one render session.
/// </summary>
/// <remarks>
/// See <c>mln_render_driver_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum RenderDriverKind : uint
{
    /// <summary>
    /// Native code owns a serial worker that initializes, drives, and tears
    /// down transferable graphics state.
    /// </summary>
    CoreWorker = 1,

    /// <summary>
    /// The host explicitly calls the narrow driver API from the thread or realm
    /// where its graphics context is current.
    /// </summary>
    CallerGraphicsThread = 2,
}
