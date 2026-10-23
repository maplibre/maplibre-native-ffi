// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// What one evaluation reads.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_inputs</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleExpressionInputs
{
    /// <summary>
    /// The value of the zoom expression.
    /// </summary>
    public double? Zoom { get; set; }

    /// <summary>
    /// The value of the heatmap-density and line-progress expressions, and of
    /// the elevation expression in a color-relief layer.
    /// </summary>
    public double? ColorRampParameter { get; set; }

    /// <summary>
    /// The value of the elevation expression outside a color-relief layer.
    /// </summary>
    public double? Elevation { get; set; }

    /// <summary>
    /// One UTF-8 GeoJSON Feature that feature data expressions read.
    /// </summary>
    public byte[]? Feature
    {
        get => FeatureStorage?.ToArray();
        set => FeatureStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? FeatureStorage { get; set; }

    /// <summary>
    /// A UTF-8 JSON object that the feature-state expression reads.
    /// </summary>
    public byte[]? FeatureState
    {
        get => FeatureStateStorage?.ToArray();
        set => FeatureStateStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? FeatureStateStorage { get; set; }

    /// <summary>
    /// A UTF-8 JSON object that the global-state expression reads.
    /// </summary>
    public byte[]? GlobalState
    {
        get => GlobalStateStorage?.ToArray();
        set => GlobalStateStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? GlobalStateStorage { get; set; }

    /// <summary>
    /// The tile that holds the feature, which the within and distance
    /// expressions need to place its geometry.
    /// </summary>
    public CanonicalTileId? CanonicalTileId { get; set; }

    /// <summary>
    /// The image IDs that the image expression reports as available. The
    /// expression reports any other ID as unavailable.
    /// </summary>
    public string[] AvailableImages
    {
        get => AvailableImagesStorage.ToArray();
        set => AvailableImagesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<string> AvailableImagesStorage { get; set; }
    public static StyleExpressionInputs Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyStyleExpressionInputs(
                NativeMethods.mln_style_expression_inputs_default()
            );
        }
    }
}
