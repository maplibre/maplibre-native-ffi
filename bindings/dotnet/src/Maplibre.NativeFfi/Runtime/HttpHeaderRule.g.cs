// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One rule of a native HTTP header transform.
/// </summary>
/// <remarks>
/// See <c>mln_http_header_rule</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct HttpHeaderRule
{
    public HttpHeaderRule()
    {
        this = Default;
    }

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
    /// The resource kinds to match. Zero selects every kind.
    /// </summary>
    public ResourceKindFlag Kinds { get; init; }
    public UrlMatchFlag Flags { get; init; }

    /// <summary>
    /// The URL or glob pattern to compare.
    /// </summary>
    public string Url { get; init; }

    /// <summary>
    /// The headers to set. Registration validates each one as
    /// <c>mln_http_header_transform_response_set_header()</c> does.
    /// </summary>
    public ImmutableArray<HttpHeader> Headers
    {
        get => HeadersStorage.AsImmutable();
        init => HeadersStorage = ValueArray.From(value);
    }
    internal ValueArray<HttpHeader> HeadersStorage { get; init; }
    public static HttpHeaderRule Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyHttpHeaderRule(NativeMethods.mln_http_header_rule_default());
        }
    }
}
