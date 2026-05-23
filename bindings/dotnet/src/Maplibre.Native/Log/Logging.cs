namespace Maplibre.Native.Log;

/// <summary>Log severity.</summary>
public enum LogSeverity : uint { Info = 1, Warning = 2, Error = 3 }

/// <summary>Log event domain.</summary>
public enum LogEvent : uint
{
    General = 1,
    Setup = 2,
    Shader = 3,
    ParseStyle = 4,
    ParseTile = 5,
    Sprite = 6,
    Image = 7,
    Glyph = 8,
    Database = 9,
    HttpRequest = 10,
    Render = 11,
    Style = 12,
    OpenGl = 13,
    Android = 14,
    Jni = 15,
    Timing = 16,
    Crash = 17,
}

/// <summary>Log severity mask.</summary>
[Flags]
public enum LogSeverityMask : uint
{
    None = 0,
    Info = 1u << 0,
    Warning = 1u << 1,
    Error = 1u << 2,
    All = Info | Warning | Error,
}

/// <summary>Copied log record.</summary>
public sealed record LogRecord(LogSeverity Severity, LogEvent Event, string? Code, string Message);

/// <summary>Log callback delegate.</summary>
public delegate void LogCallback(LogRecord record);
