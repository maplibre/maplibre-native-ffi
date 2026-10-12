// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Terminal dispositions reported by command completions.
/// </summary>
/// <remarks>
/// See <c>mln_command_disposition</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/completion_8h.html">C API reference</see>.
/// </remarks>
public enum CommandDisposition : uint
{
    Committed = 0,
    Superseded = 1,
    Failed = 2,
    Cancelled = 3,
}
