// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One source-feature query hit borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_source_feature</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct SourceFeature
{
    public SourceFeature(byte[] Feature)
    {
        this.Feature = Feature;
    }

    public byte[] Feature
    {
        get => FeatureStorage.ToArray();
        init => FeatureStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> FeatureStorage { get; init; }
}
