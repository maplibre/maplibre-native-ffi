namespace Maplibre.NativeFfi.Error;

/// <summary>The loaded C library and this binding have different ABI versions.</summary>
public sealed class AbiVersionMismatchException(uint expected, uint actual)
    : Exception(
        $"MapLibre Native C ABI version {actual} is incompatible with this binding; expected {expected}."
    )
{
    public uint Expected { get; } = expected;
    public uint Actual { get; } = actual;
}
