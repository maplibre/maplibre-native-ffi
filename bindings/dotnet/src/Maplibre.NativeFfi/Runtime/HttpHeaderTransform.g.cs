// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// HTTP header transform state: a callback, or native header rules.
/// </summary>
/// <remarks>
/// See <c>mln_http_header_transform</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct HttpHeaderTransform
{
    public HttpHeaderTransform(
        Action<ResourceKind, string, HttpHeaderTransformResponse>? Callback,
        HttpHeaderRule[] Rules
    )
    {
        this.Callback = Callback;
        this.Rules = Rules;
    }

    /// <summary>
    /// The callback, or null to apply rules.
    /// </summary>
    public Action<ResourceKind, string, HttpHeaderTransformResponse>? Callback { get; init; }

    /// <summary>
    /// Header rules for a transform without a callback; empty with one.
    /// </summary>
    public HttpHeaderRule[] Rules
    {
        get => RulesStorage.ToArray();
        init => RulesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<HttpHeaderRule> RulesStorage { get; init; }
}
