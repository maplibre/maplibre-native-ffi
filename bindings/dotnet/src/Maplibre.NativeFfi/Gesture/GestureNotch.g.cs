// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Values every <c>spacing</c> from <c>offset</c>, in the part's unit, that
/// report an <c>GestureEvent.Notch</c> as a gesture moves the part to or across
/// one, for the host to play as haptic feedback. Bearing notches lie within one
/// turn from the offset.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_notch</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Spacing">
/// Positive.
/// </param>
/// <param name="Offset">
/// Finite.
/// </param>
/// <param name="Kind">
/// The host's name for the feedback, which the notch's events carry.
/// </param>
public readonly partial record struct GestureNotch(double Spacing, double Offset, uint Kind);
