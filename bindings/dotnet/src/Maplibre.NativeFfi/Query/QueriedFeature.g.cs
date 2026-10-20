// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One query hit borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_queried_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public sealed record QueriedFeature
{
    public byte[] Feature
    {
        get => FeatureStorage.ToArray();
        set => FeatureStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> FeatureStorage { get; set; }
    public string? SourceId { get; set; }
    public string? SourceLayerId { get; set; }
    public byte[]? State
    {
        get => StateStorage?.ToArray();
        set => StateStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<byte>? StateStorage { get; set; }
}
