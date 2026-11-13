// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Tells a custom layer that a session stopped drawing it.
/// </summary>
/// <remarks>
/// <para><c>deinitialize</c> runs once a session that ran the layer's prerender
/// or render callback stops drawing the layer: when the layer leaves the style,
/// or the session detaches, is released, or replaces its device's backend.
/// Destroy the GPU objects that you made for that session then. The session's
/// GPU no longer uses them, and an OpenGL session's context is current.
/// <c>context_lost</c> runs instead when the session is abandoned. Its graphics
/// objects may be gone already, so forget yours without destroying them.</para>
/// <para>A later prerender or render callback is for a new session, which can
/// render on another device or backend.</para>
/// <para>Runs on the thread that runs the session's graphics work, as
/// <c>CustomLayerPrerenderCallback</c> says. It can't call the library.</para>
/// <para>See <c>mln_custom_layer_session_end_callback</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
public delegate void CustomLayerSessionEndCallback();
