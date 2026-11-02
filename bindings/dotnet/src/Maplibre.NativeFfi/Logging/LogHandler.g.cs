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
    public LogHandler(LogCallback Callback)
    {
        this.Callback = Callback;
    }

    public required LogCallback Callback { get; init; }
}
