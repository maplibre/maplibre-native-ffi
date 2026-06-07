namespace Maplibre.Native.Log;

/// <summary>Log severity.</summary>
public enum LogSeverity : uint
{
    Info = 1,
    Warning = 2,
    Error = 3,
}

/// <summary>Log event domain.</summary>
public enum LogEvent : uint
{
    General = 0,
    Setup = 1,
    Shader = 2,
    ParseStyle = 3,
    ParseTile = 4,
    Render = 5,
    Style = 6,
    Database = 7,
    HttpRequest = 8,
    Sprite = 9,
    Image = 10,
    OpenGl = 11,
    Jni = 12,
    Android = 13,
    Crash = 14,
    Glyph = 15,
    Timing = 16,
}

/// <summary>Log severity mask.</summary>
[Flags]
public enum LogSeverityMask : uint
{
    None = 0,
    Info = 1u << (int)LogSeverity.Info,
    Warning = 1u << (int)LogSeverity.Warning,
    Error = 1u << (int)LogSeverity.Error,
    Default = Info | Warning,
    All = Info | Warning | Error,
}

/// <summary>Copied MapLibre Native log record delivered to a log callback.</summary>
public sealed record LogRecord(
    LogSeverity Severity,
    uint RawSeverity,
    LogEvent Event,
    uint RawEvent,
    long Code,
    string Message
);

/// <summary>Log callback delegate. Return true to consume the record, or false to let native logging handle it.</summary>
public delegate bool LogCallback(LogRecord record);
