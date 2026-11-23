// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// What kind of device a pointer is.
/// </summary>
/// <remarks>
/// See <c>mln_pointer_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public enum PointerKind : uint
{
    Mouse = 0,

    /// <summary>
    /// Two touches pinch, twist, and tilt, and a held second tap zooms.
    /// </summary>
    Touch = 1,
    Pen = 2,
}
