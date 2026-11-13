// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Draws a custom layer into the map's render pass, at its place in the style.
/// </summary>
/// <remarks>
/// <para>The map draws its layers below the custom layer before the callback,
/// and those above it after, except for the opaque fragments of 2D layers,
/// which it draws before either, as
/// <c>CustomLayerRenderParameters.LayerDepth</c> describes. Draw through
/// <c>frame.frame_projection.matrix</c> to depth-test against the map's 3D
/// layers.</para>
/// <para>Runs on the thread that runs the session's graphics work, as
/// <c>CustomLayerPrerenderCallback</c> says. It can't call the library.</para>
/// <para>See <c>mln_custom_layer_render_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void CustomLayerRenderCallback(CustomLayerRenderParameters parameters);
