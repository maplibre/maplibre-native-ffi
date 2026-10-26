// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Receives a MapLibre Native log record. The callback runs synchronously on
/// the thread that emits the record, or on a MapLibre background thread for a
/// severity that <c>MaplibreNative.SetLogAsyncSeverityMask()</c> dispatches
/// asynchronously.
/// </summary>
/// <remarks>
/// <para>A thrown exception answers <c>false</c> and is reported to
/// <c>MaplibreNative.CallbackException</c>.</para>
/// <para>See <c>mln_log_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate bool LogCallback(LogSeverity severity, LogEvent @event, long code, string message);
