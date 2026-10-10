// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL backend context fields shared by OpenGL render targets.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_context_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OpenglContextDescriptor(
    OpenglContextOwnership Ownership,
    OpenglContextDescriptor.DataValue Data
)
{
    public abstract record DataValue
    {
        private DataValue() { }

        public sealed record Wgl(WglContextDescriptor Value) : DataValue;

        public sealed record Egl(EglContextDescriptor Value) : DataValue;

        public sealed record Webgl(WebglContextDescriptor Value) : DataValue;

        public sealed record Unknown : DataValue
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
}
