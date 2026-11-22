// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A pointer moved, went up, or was taken away.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_pointer</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Position">
/// Unused for <c>GestureInput.PointerCancel</c>.
/// </param>
public readonly partial record struct GesturePointer(uint Id, ScreenPoint Position);
