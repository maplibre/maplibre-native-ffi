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
    public double? Zoom { get; init; }

    /// <summary>
    /// The value of the heatmap-density and line-progress expressions, and of
    /// the elevation expression in a color-relief layer.
    /// </summary>
    public double? ColorRampParameter { get; init; }

    /// <summary>
    /// The value of the elevation expression outside a color-relief layer.
    /// </summary>
    public double? Elevation { get; init; }

    /// <summary>
    /// One UTF-8 GeoJSON Feature that feature data expressions read.
    /// </summary>
    public required string Feature { get; init; }

    /// <summary>
    /// A UTF-8 JSON object that the feature-state expression reads.
    /// </summary>
    public required string FeatureState { get; init; }

    /// <summary>
    /// A UTF-8 JSON object that the global-state expression reads.
    /// </summary>
    public required string GlobalState { get; init; }

    /// <summary>
    /// The tile that holds the feature, which the within and distance
    /// expressions need to place its geometry.
    /// </summary>
    public CanonicalTileId? CanonicalTileId { get; init; }

    /// <summary>
    /// The image IDs that the image expression reports as available. The
    /// expression reports any other ID as unavailable.
    /// </summary>
    public ImmutableArray<string> AvailableImages
    {
        get => AvailableImagesStorage.AsImmutable();
        init => AvailableImagesStorage = ValueArray.From(value);
    }
    internal ValueArray<string> AvailableImagesStorage { get; init; }
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
