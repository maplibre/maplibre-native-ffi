// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Headers to add to HTTP requests, for
/// <c>RuntimeHandle.SetHttpHeaderTransformAsync()</c>.
/// </summary>
/// <remarks>
/// <para>Set either <c>callback</c> or <c>rules</c>, not both.</para>
/// <para>Runs on a library thread.</para>
/// <para>See <c>mln_http_header_transform</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct HttpHeaderTransform
{
    public HttpHeaderTransform(
        HttpHeaderTransformCallback? Callback,
        ImmutableArray<HttpHeaderRule> Rules
    )
    {
        this.Callback = Callback;
        this.Rules = Rules;
    }

    public HttpHeaderTransformCallback? Callback { get; init; }

    /// <summary>
    /// The first rule that matches a request adds its headers, and no other.
    /// </summary>
    public ImmutableArray<HttpHeaderRule> Rules
    {
        get => RulesStorage.AsImmutable();
        init => RulesStorage = ValueArray.From(value);
    }
    internal ValueArray<HttpHeaderRule> RulesStorage { get; init; }
}
