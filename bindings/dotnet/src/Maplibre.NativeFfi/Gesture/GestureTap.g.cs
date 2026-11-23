// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A tap, at <c>position</c>, by the pointer <c>id</c>.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_tap</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Id">
/// Unused for <c>GestureEvent.TwoFingerTap</c>.
/// </param>
public readonly partial record struct GestureTap(uint Id, ScreenPoint Position);
