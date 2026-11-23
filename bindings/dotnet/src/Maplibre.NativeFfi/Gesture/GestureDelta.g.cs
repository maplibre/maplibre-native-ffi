// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Camera movement from anything but a pointer, such as a wheel, a trackpad
/// scroll, a dial, a crown, keys, or buttons, which the host converts into
/// camera units. Zooms and turns hold <c>position</c> in place.
/// </summary>
/// <remarks>
/// <para>A continuous delta, such as a trackpad scroll, applies at once. A
/// discrete one, such as a wheel detent or a key press, eases, and a quick run
/// of them merges into one ease that speeds up. Deltas are one gesture until
/// they idle. The recommended amounts are a third of a level per wheel detent,
/// 1/300 of a level per pixel of trackpad scrolling, and, for keys, 100 logical
/// pixels of pan, a level of zoom, 15 degrees of bearing, and 10 of
/// pitch.</para>
/// <para>See <c>mln_gesture_delta</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Pan">
/// How far the map's content moves on the screen, in logical pixels.
/// </param>
/// <param name="Zoom">
/// Zoom levels to add.
/// </param>
/// <param name="Bearing">
/// Degrees to turn clockwise.
/// </param>
/// <param name="Pitch">
/// Degrees to tilt.
/// </param>
public readonly partial record struct GestureDelta(
    ScreenPoint Position,
    bool Discrete,
    ScreenPoint Pan,
    double Zoom,
    double Bearing,
    double Pitch
);
