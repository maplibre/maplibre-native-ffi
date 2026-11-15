// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What a custom layer's prerender callback gets.
/// </summary>
/// <remarks>
/// See <c>mln_custom_layer_prerender_parameters</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct CustomLayerPrerenderParameters(
    CustomLayerPrerenderParameters.EncoderValue Encoder,
    CustomLayerFrame Frame
)
{
    public abstract record EncoderValue
    {
        private EncoderValue() { }

        public sealed record Metal(MetalCustomLayerEncoder Value) : EncoderValue;

        public sealed record Vulkan(VulkanCustomLayerEncoder Value) : EncoderValue;

        public sealed record WebGpu(WebGpuCustomLayerEncoder Value) : EncoderValue;

        public sealed record None : EncoderValue;

        public sealed record Unknown : EncoderValue
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
