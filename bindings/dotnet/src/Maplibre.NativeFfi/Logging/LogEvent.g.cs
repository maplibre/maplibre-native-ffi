// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Log event categories emitted by MapLibre Native.
/// </summary>
/// <remarks>
/// See <c>mln_log_event</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html">C API reference</see>.
/// </remarks>
public enum LogEvent : uint
{
    General = 0,
    Setup = 1,
    Shader = 2,
    ParseStyle = 3,
    ParseTile = 4,
    Render = 5,
    Style = 6,
    Database = 7,
    HttpRequest = 8,
    Sprite = 9,
    Image = 10,
    GraphicsBackend = 11,
    Jni = 12,
    Android = 13,
    Crash = 14,
    Glyph = 15,
    Timing = 16,
}
