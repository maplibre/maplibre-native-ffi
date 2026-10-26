// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Process-global log callback. The callback runs synchronously on the thread
/// that emits the record, or on a MapLibre background thread for a severity
/// that <c>MaplibreNative.SetLogAsyncSeverityMask()</c> dispatches
/// asynchronously.
/// </summary>
/// <remarks>
/// See <c>mln_log_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct LogHandler
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public LogHandler(LogCallback Callback)
    {
        this.Callback = Callback;
    }

    /// <summary>
    /// Receives each record.
    /// </summary>
    public required LogCallback Callback { get; init; }
}
