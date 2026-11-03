// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The features whose state to set, read, or remove.
/// </summary>
/// <remarks>
/// See <c>mln_feature_state_selector</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record FeatureStateSelector
{
    public required string SourceId { get; init; }

    /// <summary>
    /// The source layer, for a vector source. Empty for other sources.
    /// </summary>
    public string SourceLayerId { get; init; } = "";

    /// <summary>
    /// The feature's ID. Setting or reading state needs it. Without it,
    /// <c>MapHandle.RemoveFeatureStateAsync()</c> removes the state of every
    /// feature in the source layer.
    /// </summary>
    public FeatureId? FeatureId { get; init; }

    /// <summary>
    /// The one key that <c>MapHandle.RemoveFeatureStateAsync()</c> removes. It
    /// needs <c>feature_id</c>. Empty removes every key.
    /// </summary>
    public string StateKey { get; init; } = "";
}
