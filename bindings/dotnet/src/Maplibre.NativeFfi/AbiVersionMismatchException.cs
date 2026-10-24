namespace Maplibre.NativeFfi;

/// <summary>
/// The loaded C library and this binding have different ABI versions, so the library is
/// <see cref="Status.Unsupported" />.
/// </summary>
public sealed class AbiVersionMismatchException(uint expected, uint actual)
    : MaplibreException(
        Status.Unsupported,
        $"MapLibre Native C ABI version {actual} is incompatible with this binding; expected {expected}."
    )
{
    public uint Expected { get; } = expected;
    public uint Actual { get; } = actual;
}
