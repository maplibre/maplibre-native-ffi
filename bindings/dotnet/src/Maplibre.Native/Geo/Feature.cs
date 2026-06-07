using Maplibre.Native.Json;

namespace Maplibre.Native.Geo;

/// <summary>Feature identifier value.</summary>
public abstract record FeatureIdentifier
{
    private FeatureIdentifier() { }

    public sealed record Null : FeatureIdentifier
    {
        public static Null Instance { get; } = new();

        private Null() { }
    }

    public sealed record UInt(ulong Value) : FeatureIdentifier;

    public sealed record Int(long Value) : FeatureIdentifier;

    public sealed record Double(double Value) : FeatureIdentifier;

    public sealed record String(string Value) : FeatureIdentifier;
}

/// <summary>GeoJSON feature value.</summary>
public sealed record Feature(
    Geometry Geometry,
    IReadOnlyList<JsonMember> Properties,
    FeatureIdentifier Identifier
);

/// <summary>GeoJSON value.</summary>
public abstract record GeoJson
{
    private GeoJson() { }

    public sealed record GeometryValue(Geometry Geometry) : GeoJson;

    public sealed record FeatureValue(Feature Feature) : GeoJson;

    public sealed record FeatureCollection(IReadOnlyList<Feature> Features) : GeoJson;
}
