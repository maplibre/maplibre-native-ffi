// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A pointer went down. The host sends no pointer whose buttons mean nothing,
/// such as a mouse's middle button.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_pointer_down</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Modes">
/// The modes of the pointer's held buttons.
/// </param>
public readonly partial record struct GesturePointerDown(
    uint Id,
    PointerKind Kind,
    ScreenPoint Position,
    GestureMode Modes
);
