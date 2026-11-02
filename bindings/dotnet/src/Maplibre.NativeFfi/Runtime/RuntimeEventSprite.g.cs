// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The details of a sprite event.
/// </summary>
/// <remarks>
/// <para><c>id</c> and <c>url</c> are both empty when the style has no
/// sprite.</para>
/// <para>See <c>mln_runtime_event_sprite</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct RuntimeEventSprite(string Id, string Url);
