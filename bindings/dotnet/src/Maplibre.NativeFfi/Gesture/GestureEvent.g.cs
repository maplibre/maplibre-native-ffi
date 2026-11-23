// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What a gesture means beyond moving the camera. Double taps and two-finger
/// taps also zoom, as the options say.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_event</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public abstract record GestureEvent
{
    private GestureEvent() { }

    public sealed record Tap(GestureTap Value) : GestureEvent;

    public sealed record DoubleTap(GestureTap Value) : GestureEvent;

    public sealed record TwoFingerTap(GestureTap Value) : GestureEvent;

    public sealed record Notch(GestureNotchEvent Value) : GestureEvent;

    public sealed record LongPress(GestureTap Value) : GestureEvent;

    public sealed record Unknown : GestureEvent
    {
        private readonly byte[] bytes;
        public uint Tag { get; }
        public byte[] PayloadBytes => (byte[])bytes.Clone();

        public Unknown(uint tag, byte[] payloadBytes)
        {
            Tag = tag;
            bytes = (byte[])payloadBytes.Clone();
        }

        public bool Equals(Unknown? other) =>
            other is not null && Tag == other.Tag && bytes.AsSpan().SequenceEqual(other.bytes);

        public override int GetHashCode()
        {
            var hash = new HashCode();
            hash.Add(Tag);
            foreach (var value in bytes)
                hash.Add(value);
            return hash.ToHashCode();
        }
    }
}
