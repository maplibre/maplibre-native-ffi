// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A query parameter that a resource transform rule adds to a URL.
/// </summary>
/// <remarks>
/// <para>Pass <c>name</c> and <c>value</c> unencoded. The library
/// percent-encodes every byte of them except the letters, digits, <c>-</c>,
/// <c>.</c>, <c>_</c>, and <c>~</c>, and adds them as <c>name=value</c>.</para>
/// <para>See <c>mln_url_query_parameter</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct UrlQueryParameter
{
    public UrlQueryParameter() { }

    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public UrlQueryParameter(string Name, string Value)
    {
        this.Name = Name;
        this.Value = Value;
    }

    public required string Name { get; init; }
    public string Value { get; init; } = "";
}
