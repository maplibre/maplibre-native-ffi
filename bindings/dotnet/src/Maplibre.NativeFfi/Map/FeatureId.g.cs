// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The ID of a feature in a source, as the source's data gives it.
/// </summary>
/// <remarks>
/// <para>The map compares feature IDs as text. A number names the same feature
/// as the string of its shortest decimal form, so 7, 7.0, and "7" name one
/// feature.</para>
/// <para>See <c>mln_feature_id</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public abstract record FeatureId
{
    private FeatureId() { }

    public sealed record StringValue(string Value) : FeatureId;

    public sealed record UintValue(ulong Value) : FeatureId;

    public sealed record IntValue(long Value) : FeatureId;

    /// <param name="Value">
    /// A finite number.
    /// </param>
    public sealed record DoubleValue(double Value) : FeatureId;
}
