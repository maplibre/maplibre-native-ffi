// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Process-global log callback state.
/// </summary>
/// <remarks>
/// See <c>mln_log_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct LogHandler(
    Func<LogSeverity, LogEvent, long, string, uint>? Callback
);
