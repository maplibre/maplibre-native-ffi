// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Map;

public sealed record FeatureStateSelector
{
    public required string SourceId { get; set; }
    public string? SourceLayerId { get; set; }
    public string? FeatureId { get; set; }
    public string? StateKey { get; set; }
}
