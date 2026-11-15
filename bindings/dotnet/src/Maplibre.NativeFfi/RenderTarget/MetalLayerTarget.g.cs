// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A <c>CAMetalLayer</c> to present to.
/// </summary>
/// <remarks>
/// <para>The session sets the layer's device, pixel format, and drawable
/// size.</para>
/// <para>See <c>mln_metal_layer_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct MetalLayerTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalLayerTarget(LogicalSize LogicalSize, NativePointer Layer)
    {
        this.LogicalSize = LogicalSize;
        this.Layer = Layer;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>CAMetalLayer*</c>.
    /// </summary>
    public required NativePointer Layer { get; init; }
}
