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
    public HttpHeaderRule(uint Kind, UrlMatchFlags Flags, string Url, HttpHeader[] Headers)
    {
        this.Kind = Kind;
        this.Flags = Flags;
        this.Url = Url;
        this.Headers = Headers;
    }

    /// <summary>
    /// One of <c>mln_resource_kind</c>, or <c>MLN_RESOURCE_KIND_ANY</c>.
    /// </summary>
    public uint Kind { get; init; }

    /// <summary>
    /// A bitwise OR of <c>mln_url_match_flags</c> values other than
    /// REQUESTED_URL.
    /// </summary>
    public UrlMatchFlags Flags { get; init; }

    /// <summary>
    /// The URL or glob pattern to compare.
    /// </summary>
    public string Url { get; init; }

    /// <summary>
    /// The headers to set. Registration validates each one as
    /// <c>mln_http_header_transform_response_set_header()</c> does.
    /// </summary>
    public HttpHeader[] Headers
    {
        get => HeadersStorage.ToArray();
        init => HeadersStorage = ValueArray.Copy(value);
    }
    internal ValueArray<HttpHeader> HeadersStorage { get; init; }
}
