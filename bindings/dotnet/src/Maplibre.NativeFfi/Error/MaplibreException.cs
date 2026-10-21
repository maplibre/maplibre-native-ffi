namespace Maplibre.NativeFfi.Error;

/// <summary>A native or binding status failure.</summary>
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
}
