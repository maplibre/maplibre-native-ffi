// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Receives a log message.
/// </summary>
/// <remarks>
/// <para>Runs on the thread that logged the message, or on a background thread
/// for the severities that <c>MaplibreNative.SetLogAsyncSeverityMask()</c>
/// selects. The callback can't call the library.</para>
/// <para>A thrown exception answers <c>false</c> and is reported to
/// <c>MaplibreNative.CallbackException</c>.</para>
/// <para>See <c>mln_log_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate bool LogCallback(LogSeverity severity, LogEvent @event, long code, string message);
