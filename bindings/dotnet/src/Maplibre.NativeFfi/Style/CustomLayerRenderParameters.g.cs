// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What a custom layer's render callback gets.
/// </summary>
/// <remarks>
/// See <c>mln_custom_layer_render_parameters</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct CustomLayerRenderParameters
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public CustomLayerRenderParameters(
        CustomLayerRenderParameters.PassValue Pass,
        CustomLayerFrame Frame,
        double LayerDepth
    )
    {
        this.Pass = Pass;
        this.Frame = Frame;
        this.LayerDepth = LayerDepth;
    }

    public required CustomLayerRenderParameters.PassValue Pass { get; init; }
    public CustomLayerFrame Frame { get; init; }

    /// <summary>
    /// The depth that a 2D layer at the layer's place in the style writes, as
    /// <c>FrameProjection</c> describes, between
    /// <c>frame.frame_projection.far_depth</c> and 1.
    /// </summary>
    /// <remarks>
    /// The map draws the opaque fragments of its 2D layers before the render
    /// callback runs, those above the custom layer too. A fragment that you
    /// draw at this depth, tested with less-or-equal, stays under those above
    /// that wrote depth.
    /// </remarks>
    public double LayerDepth { get; init; }

    public abstract record PassValue
    {
        private PassValue() { }

        public sealed record Metal(MetalCustomLayerPass Value) : PassValue;

        public sealed record Vulkan(VulkanCustomLayerPass Value) : PassValue;

        public sealed record OpenGl(OpenGlCustomLayerPass Value) : PassValue;

        public sealed record WebGpu(WebGpuCustomLayerPass Value) : PassValue;

        public sealed record Unknown : PassValue
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
