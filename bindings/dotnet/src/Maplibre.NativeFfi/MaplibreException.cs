namespace Maplibre.NativeFfi;

/// <summary>
/// A native status and the diagnostic that native copied for it. A failed or cancelled command
/// faults its task with this exception.
/// </summary>
public class MaplibreException : Exception
{
    public MaplibreException(Status status, string diagnostic)
        : this(status, diagnostic, null) { }

    public MaplibreException(Status status, string diagnostic, Exception? innerException)
        : base($"{status}: {diagnostic}", innerException)
    {
        Status = status;
        Diagnostic = diagnostic;
    }

    public Status Status { get; }
    public string Diagnostic { get; }

    /// <summary>
    /// The map snapshot generation that a failed or cancelled map command published, and zero for
    /// any other failure.
    /// </summary>
    public ulong Generation { get; init; }
}
