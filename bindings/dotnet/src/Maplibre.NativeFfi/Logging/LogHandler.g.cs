// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A log callback, for <c>MaplibreNative.SetLogCallback()</c>.
/// </summary>
/// <remarks>
/// <para>Runs on the thread that logged the message, or on a background thread
/// for the severities that <c>MaplibreNative.SetLogAsyncSeverityMask()</c>
/// selects. The callback can't call the library.</para>
/// <para>See <c>mln_log_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct LogHandler
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public LogHandler(LogCallback Callback, LogSeverityFlag PlatformSeverities)
    {
        this.Callback = Callback;
        this.PlatformSeverities = PlatformSeverities;
    }

    public required LogCallback Callback { get; init; }

    /// <summary>
    /// The severities that also go to the platform's log, as they do without a
    /// callback. 0 sends none.
    /// </summary>
    /// <remarks>
    /// A record that the handler's delivery queue can't accept is dropped.
    /// These severities alone decide whether a record also goes to the
    /// platform's log, whether or not the queue accepts it.
    /// </remarks>
    public LogSeverityFlag PlatformSeverities { get; init; }
}
