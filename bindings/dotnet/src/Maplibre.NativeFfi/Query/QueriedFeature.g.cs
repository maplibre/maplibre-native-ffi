// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Query;

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
