namespace Maplibre.Native.Error;

/// <summary>Exception for operations that are invalid in the current lifecycle state.</summary>
public sealed class InvalidStateException : MaplibreException
{
    public InvalidStateException(
        MaplibreStatus status,
        int? rawStatus,
        string diagnostic,
        Exception? innerException
    )
        : base(status, rawStatus, diagnostic, innerException) { }
}
