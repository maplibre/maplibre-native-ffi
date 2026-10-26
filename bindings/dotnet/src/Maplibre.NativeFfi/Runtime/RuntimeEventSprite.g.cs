// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>RuntimeEventType.MapSpriteRequested</c>,
/// <c>RuntimeEventType.MapSpriteLoaded</c>, and
/// <c>RuntimeEventType.MapSpriteError</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_sprite</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Id">
/// Sprite ID: the id of an entry in the style's sprite array, or "default" when
/// the style names one sprite URL.
/// </param>
/// <param name="Url">
/// Sprite URL as the style names it, without the pixel-ratio suffix and file
/// extension that MapLibre appends to request the image and JSON.
/// </param>
public readonly partial record struct RuntimeEventSprite(string Id, string Url);
