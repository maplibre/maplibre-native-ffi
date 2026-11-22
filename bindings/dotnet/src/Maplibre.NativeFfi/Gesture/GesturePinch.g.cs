// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A pinch the platform recognized, such as a macOS trackpad's magnify and
/// rotate or a browser's Ctrl+wheel. Each update zooms by <c>scale</c> (above 1
/// zooms in) and turns <c>rotation</c> degrees clockwise on the screen since
/// the last, around <c>center</c>. It glides on when it ends, as a touch pinch
/// does.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_pinch</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct GesturePinch(
    ScreenPoint Center,
    double Scale,
    double Rotation,
    PinchPhase Phase
);
