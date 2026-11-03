// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The inputs that a style expression reads while it evaluates.
/// </summary>
/// <remarks>
/// See <c>mln_style_expression_inputs</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleExpressionInputs
{
    public double? Zoom { get; init; }

    /// <summary>
    /// The value of <c>heatmap-density</c>, <c>line-progress</c>, and
    /// <c>elevation</c>.
    /// </summary>
    public double? ColorRampParameter { get; init; }

    /// <summary>
    /// The feature to evaluate against, as a GeoJSON feature, or empty for
    /// none.
    /// </summary>
    public string Feature { get; init; } = "";

    /// <summary>
    /// The feature's state, as a JSON object, or empty for none.
    /// </summary>
    public string FeatureState { get; init; } = "";

    /// <summary>
    /// The values that <c>global-state</c> reads, as a JSON object, or empty
    /// for none.
    /// </summary>
    public string GlobalState { get; init; } = "";

    /// <summary>
    /// The tile that holds the feature. <c>within</c> and <c>distance</c> need
    /// it to read the feature's geometry.
    /// </summary>
    public CanonicalTileId? CanonicalTileId { get; init; }

    /// <summary>
    /// The IDs of the images that <c>image</c> treats as available.
    /// </summary>
    public ImmutableArray<string> AvailableImages
    {
        get => AvailableImagesStorage.AsImmutable();
        init => AvailableImagesStorage = ValueArray.From(value);
    }
    internal ValueArray<string> AvailableImagesStorage { get; init; }
}
