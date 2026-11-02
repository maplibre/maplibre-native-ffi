// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// A log severity, as a bit in a set.
/// </summary>
/// <remarks>
/// See <c>mln_log_severity_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum LogSeverityFlag : uint
{
    Info = 2,
    Warning = 4,
    Error = 8,

    /// <summary>
    /// The severities logged on a background thread until
    /// <c>MaplibreNative.SetLogAsyncSeverityMask()</c> changes them.
    /// </summary>
    Default = 6,
    All = 14,
}
