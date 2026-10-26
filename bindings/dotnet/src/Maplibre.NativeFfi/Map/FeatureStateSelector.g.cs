// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Feature-state source, feature, and key selector.
/// </summary>
/// <remarks>
/// See <c>mln_feature_state_selector</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record FeatureStateSelector
{
    /// <summary>
    /// Source ID. Required.
    /// </summary>
    public required string SourceId { get; init; }

    /// <summary>
    /// Source layer ID, or empty for none. Required for vector-source
    /// disambiguation.
    /// </summary>
    public string SourceLayerId { get; init; } = "";

    /// <summary>
    /// Optional feature ID. Required by set and get, optional for remove. A
    /// numeric feature ID matches its decimal text, such as "7" for the GeoJSON
    /// id 7. Its field bit tells an absent ID from the empty ID "".
    /// </summary>
    public string? FeatureId { get; init; }

    /// <summary>
    /// State key, or empty to select every key. Used only by remove, and
    /// requires feature_id.
    /// </summary>
    public string StateKey { get; init; } = "";
}
