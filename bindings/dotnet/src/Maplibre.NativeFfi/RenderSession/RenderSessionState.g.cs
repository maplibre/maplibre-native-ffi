// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a render session is in its life.
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
    Failed = 5,
    Abandoned = 6,
}
