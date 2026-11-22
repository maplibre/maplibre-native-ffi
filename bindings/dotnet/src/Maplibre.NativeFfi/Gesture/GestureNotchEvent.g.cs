// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A notch that a part reached.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_notch_event</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Kind">
/// The notch's kind, from the options.
/// </param>
/// <param name="Value">
/// In the part's unit; a bearing from 0 up to 360 degrees.
/// </param>
public readonly partial record struct GestureNotchEvent(GestureAxis Axis, uint Kind, double Value);
