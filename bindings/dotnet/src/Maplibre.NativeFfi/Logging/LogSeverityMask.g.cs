// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Bitmask values for log severities dispatched asynchronously.
/// </summary>
/// <remarks>
/// See <c>mln_log_severity_mask</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum LogSeverityMask : uint
{
    Info = 2,
    Warning = 4,
    Error = 8,
    Default = 6,
    All = 14,
}
