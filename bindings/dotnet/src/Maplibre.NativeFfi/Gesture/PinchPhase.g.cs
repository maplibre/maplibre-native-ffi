// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a platform-recognized pinch is.
/// </summary>
/// <remarks>
/// See <c>mln_pinch_phase</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public enum PinchPhase : uint
{
    Begin = 0,
    Update = 1,

    /// <summary>
    /// The fingers lifted: the pinch glides on.
    /// </summary>
    End = 2,

    /// <summary>
    /// The platform took the pinch away: nothing continues, and the camera's
    /// animations stop.
    /// </summary>
    Cancel = 3,
}
