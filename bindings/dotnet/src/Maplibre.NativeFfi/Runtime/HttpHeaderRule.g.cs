// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Headers to add to the HTTP requests whose URL matches a pattern.
/// </summary>
/// <remarks>
/// See <c>mln_http_header_rule</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct HttpHeaderRule
{
    public HttpHeaderRule() { }

    public HttpHeaderRule(
        ResourceKindFlag Kinds,
        UrlMatchFlag Flags,
        string Url,
        ImmutableArray<HttpHeader> Headers
    )
    {
        this.Kinds = Kinds;
        this.Flags = Flags;
        this.Url = Url;
        this.Headers = Headers;
    }

    /// <summary>
    /// The kinds of resource that the rule covers. Zero covers every kind.
    /// </summary>
    public ResourceKindFlag Kinds { get; init; }
    public UrlMatchFlag Flags { get; init; }
    public string Url { get; init; } = "";
    public ImmutableArray<HttpHeader> Headers
    {
        get => HeadersStorage.AsImmutable();
        init => HeadersStorage = ValueArray.From(value);
    }
    internal ValueArray<HttpHeader> HeadersStorage { get; init; }
}
