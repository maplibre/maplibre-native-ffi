using Maplibre.Native.Geo;
using Maplibre.Native.Json;

namespace Maplibre.Native.Query;

public sealed class FeatureStateSelector
{
    public required string SourceId { get; set; }
    public string? SourceLayerId { get; set; }
    public string? FeatureId { get; set; }
    public string? StateKey { get; set; }
}

public abstract record RenderedQueryGeometry
{
    private RenderedQueryGeometry() { }

    public sealed record Point(ScreenPoint Value) : RenderedQueryGeometry;

    public sealed record Box(ScreenBox Value) : RenderedQueryGeometry;

    public sealed record LineString(IReadOnlyList<ScreenPoint> Points) : RenderedQueryGeometry;
}

public sealed class RenderedFeatureQueryOptions
{
    public IReadOnlyList<string>? LayerIds { get; set; }
    public JsonValue? Filter { get; set; }
}

public sealed class SourceFeatureQueryOptions
{
    public IReadOnlyList<string>? SourceLayerIds { get; set; }
    public JsonValue? Filter { get; set; }
}

public sealed record QueriedFeature(
    Feature Feature,
    string? SourceId,
    string? SourceLayerId,
    JsonValue? State
);

public abstract record FeatureExtensionResult
{
    private FeatureExtensionResult() { }

    public sealed record Value(JsonValue Json) : FeatureExtensionResult;

    public sealed record FeatureCollection(IReadOnlyList<Feature> Features)
        : FeatureExtensionResult;

    public sealed record Unknown(uint RawType) : FeatureExtensionResult;
}
