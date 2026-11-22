// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// What a one-pointer drag or a double tap does instead of its default, from
/// the pointer's held buttons or from held keys. A drag reads its modes when it
/// starts and keeps them until it ends, and a double tap when it lifts;
/// rotating wins over a box zoom.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum GestureMode : uint
{
    /// <summary>
    /// Sideways turns the map and up and down tilts it.
    /// </summary>
    Rotate = 1,

    /// <summary>
    /// Draws a box that the map zooms to when the pointer lifts.
    /// </summary>
    BoxZoom = 2,

    /// <summary>
    /// A double tap zooms out instead of in.
    /// </summary>
    ZoomOut = 4,
}
