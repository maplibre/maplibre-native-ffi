namespace Maplibre.Native.Error;

/// <summary>Exception for native MapLibre errors converted to C status.</summary>
public sealed class NativeErrorException : MaplibreException
{
    public NativeErrorException(
        MaplibreStatus status,
        int? rawStatus,
        string diagnostic,
        Exception? innerException = null
    )
        : base(status, rawStatus, diagnostic, innerException) { }
}
