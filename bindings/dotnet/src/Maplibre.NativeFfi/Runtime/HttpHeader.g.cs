// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An HTTP header.
/// </summary>
/// <remarks>
/// <para><c>name</c> follows the rules of
/// <c>HttpHeaderTransformResponse.SetHeader()</c>.</para>
/// <para>See <c>mln_http_header</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct HttpHeader
{
    public HttpHeader() { }

    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public HttpHeader(string Name, string Value)
    {
        this.Name = Name;
        this.Value = Value;
    }

    public required string Name { get; init; }
    public string Value { get; init; } = "";
}
