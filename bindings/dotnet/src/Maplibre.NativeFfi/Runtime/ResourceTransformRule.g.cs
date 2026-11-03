// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Query parameters to add to the URLs of requests that match a pattern.
/// </summary>
/// <remarks>
/// <para>The library adds the parameters, in order, at the end of the URL's
/// query and before any fragment. It writes <c>?</c> before them when the URL
/// has no <c>?</c>, or <c>&amp;</c> when the text before them ends with neither
/// <c>?</c> nor <c>&amp;</c>. It keeps the parameters that the URL already has,
/// even one with the same name.</para>
/// <para>See <c>mln_resource_transform_rule</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct ResourceTransformRule
{
    public ResourceTransformRule() { }

    public ResourceTransformRule(
        ResourceKindFlag Kinds,
        UrlMatchFlag Flags,
        string Url,
        ImmutableArray<UrlQueryParameter> Parameters
    )
    {
        this.Kinds = Kinds;
        this.Flags = Flags;
        this.Url = Url;
        this.Parameters = Parameters;
    }

    /// <summary>
    /// The kinds of resource that the rule covers. Zero covers every kind.
    /// </summary>
    public ResourceKindFlag Kinds { get; init; }
    public UrlMatchFlag Flags { get; init; }
    public string Url { get; init; } = "";
    public ImmutableArray<UrlQueryParameter> Parameters
    {
        get => ParametersStorage.AsImmutable();
        init => ParametersStorage = ValueArray.From(value);
    }
    internal ValueArray<UrlQueryParameter> ParametersStorage { get; init; }
}
