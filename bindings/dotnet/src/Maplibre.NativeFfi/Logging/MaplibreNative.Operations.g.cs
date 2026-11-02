// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Sends log messages to the platform's log again, instead of a callback.
    /// </summary>
    /// <remarks>
    /// See <c>mln_log_clear_callback</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
    /// </remarks>
    public static void ClearLogCallback()
    {
        using var call = Enter(null);
        Check(NativeMethods.mln_log_clear_callback(Diagnostic));
    }

    /// <summary>
    /// Selects the severities that the library logs on a background thread.
    /// </summary>
    /// <remarks>
    /// <para>The library logs other severities on the thread that logged them.
    /// <c>mask</c> applies to every runtime in the process.</para>
    /// <para>See <c>mln_log_set_async_severity_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.</para>
    /// </remarks>
    public static void SetLogAsyncSeverityMask(LogSeverityFlag mask)
    {
        using var call = Enter(null);
        Check(NativeMethods.mln_log_set_async_severity_mask((uint)mask, Diagnostic));
    }

    /// <summary>
    /// Sends the log messages of every runtime in the process to a callback.
    /// </summary>
    /// <remarks>
    /// <para>The callback replaces the one set before.</para>
    /// <para>Runs on the thread that logged the message, or on a background
    /// thread for the severities that
    /// <c>MaplibreNative.SetLogAsyncSeverityMask()</c> selects. The callback
    /// can't call the library.</para>
    /// <para>See <c>mln_log_set_callback</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.</para>
    /// </remarks>
    public static void SetLogCallback(LogHandler handler)
    {
        using var scope = new NativeCallScope(null);
        var nativeHandler = NativeLogHandler(handler, scope);
        Check(NativeMethods.mln_log_set_callback(&nativeHandler, Diagnostic));
        scope.Accept();
    }
}
