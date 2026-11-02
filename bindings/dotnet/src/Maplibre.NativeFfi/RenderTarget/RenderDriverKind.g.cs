// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The thread that runs a render session's graphics work.
/// </summary>
/// <remarks>
/// See <c>mln_render_driver_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public enum RenderDriverKind : uint
{
    /// <summary>
    /// A library thread.
    /// </summary>
    CoreWorker = 1,

    /// <summary>
    /// Your thread, when it calls
    /// <c>RenderSessionHandle.ServiceDriverWork()</c>.
    /// </summary>
    CallerGraphicsThread = 2,
}
