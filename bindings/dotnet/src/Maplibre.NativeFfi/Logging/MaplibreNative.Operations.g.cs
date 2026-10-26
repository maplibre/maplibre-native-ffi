// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class MaplibreNative
{
    /// <summary>
    /// Clears the process-global log callback.
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
    /// Controls which log severities MapLibre Native may dispatch
    /// asynchronously.
    /// </summary>
    /// <remarks>
    /// See <c>mln_log_set_async_severity_mask</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
    /// </remarks>
    public static void SetLogAsyncSeverityMask(LogSeverityFlag mask)
    {
        using var call = Enter(null);
        Check(NativeMethods.mln_log_set_async_severity_mask((uint)mask, Diagnostic));
    }

    /// <summary>
    /// Installs a process-global MapLibre Native log callback. The callback
    /// runs synchronously on the thread that emits the record, or on a MapLibre
    /// background thread for a severity that
    /// <c>MaplibreNative.SetLogAsyncSeverityMask()</c> dispatches
    /// asynchronously.
    /// </summary>
    /// <remarks>
    /// See <c>mln_log_set_callback</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
    /// </remarks>
    public static void SetLogCallback(LogHandler handler)
    {
        using var scope = new NativeCallScope(null);
        var nativeHandler = NativeLogHandler(handler, scope);
        Check(NativeMethods.mln_log_set_callback(&nativeHandler, Diagnostic));
        scope.Accept();
    }
}
