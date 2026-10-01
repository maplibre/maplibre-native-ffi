// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Query;

public sealed record QueriedFeature
{
    private byte[]? storageFeature;
    public byte[] Feature
    {
        get => storageFeature?.ToArray() ?? [];
        set => storageFeature = value?.ToArray() ?? [];
    }
    internal byte[] FeatureStorage
    {
        get => storageFeature ?? [];
        init => storageFeature = value;
    }
    public string? SourceId { get; set; }
    public string? SourceLayerId { get; set; }
    private byte[]? storageState;
    public byte[]? State
    {
        get => storageState?.ToArray();
        set => storageState = value?.ToArray();
    }
    internal byte[]? StateStorage
    {
        get => storageState;
        init => storageState = value;
    }

    public bool Equals(QueriedFeature? other) =>
        other is not null
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            FeatureStorage,
            other.FeatureStorage
        )
        && EqualityComparer<string?>.Default.Equals(SourceId, other.SourceId)
        && EqualityComparer<string?>.Default.Equals(SourceLayerId, other.SourceLayerId)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            StateStorage,
            other.StateStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(FeatureStorage)
        );
        hash.Add(SourceId);
        hash.Add(SourceLayerId);
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(StateStorage));
        return hash.ToHashCode();
    }
}
