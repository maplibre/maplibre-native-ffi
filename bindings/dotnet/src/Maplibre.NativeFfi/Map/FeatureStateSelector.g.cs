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
    /// Source ID. Required and borrowed for the duration of the call.
    /// </summary>
    public required string SourceId { get; set; }

    /// <summary>
    /// Optional source layer ID. Required for vector-source disambiguation.
    /// </summary>
    public string? SourceLayerId { get; set; }

    /// <summary>
    /// Optional feature ID string. Required by set/get and optional for remove.
    /// </summary>
    public string? FeatureId { get; set; }

    /// <summary>
    /// Optional state key. Used only by remove and requires feature_id.
    /// </summary>
    public string? StateKey { get; set; }
}
