// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Records a custom layer's GPU work that can't run in the map's render pass,
/// such as drawing into textures of your own.
/// </summary>
/// <remarks>
/// <para>Each frame that shows the layer runs the prerender callbacks of its
/// custom layers, from the bottom of the style to the top, before the map
/// begins its render pass, and so before any render callback.</para>
/// <para>Runs on the thread that runs the session's graphics work: a library
/// thread, or yours with <c>RenderDriverKind.CallerGraphicsThread</c>. It can't
/// call the library.</para>
/// <para>See <c>mln_custom_layer_prerender_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void CustomLayerPrerenderCallback(CustomLayerPrerenderParameters parameters);
