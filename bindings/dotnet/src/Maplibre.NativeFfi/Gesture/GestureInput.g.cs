// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One input, in logical pixels from the map's top-left corner.
/// </summary>
/// <remarks>
/// <para>When the view loses focus, the host cancels the pointers it sent and
/// sends empty modes.</para>
/// <para>See <c>mln_gesture_input</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.</para>
/// </remarks>
public abstract record GestureInput
{
    private GestureInput() { }

    public sealed record PointerDown(GesturePointerDown Value) : GestureInput;

    public sealed record PointerMove(GesturePointer Value) : GestureInput;

    public sealed record PointerUp(GesturePointer Value) : GestureInput;

    public sealed record PointerCancel(GesturePointer Value) : GestureInput;

    public sealed record Modes(GestureMode Value) : GestureInput;

    public sealed record Delta(GestureDelta Value) : GestureInput;

    public sealed record Pinch(GesturePinch Value) : GestureInput;
}
