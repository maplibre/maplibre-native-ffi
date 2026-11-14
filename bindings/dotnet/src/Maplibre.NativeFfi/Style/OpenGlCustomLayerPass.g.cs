// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The map's OpenGL framebuffer, at a custom layer's place in the style.
/// </summary>
/// <remarks>
/// <para>The session's context is current, with <c>framebuffer</c> bound and
/// the viewport covering it. You can change the program, the vertex array, the
/// buffers bound to each target, the framebuffer, the viewport, the active
/// texture unit and the textures bound to each unit, the uniform buffer
/// bindings, and the scissor, depth, stencil, blend, color mask, cull face, and
/// line width state. After the callback returns, the map sets again what its
/// own draws need of these. The vertex array bound when the callback starts is
/// 0, so bind your own before you set vertex attributes. Leave any other state
/// as you found it, such as the sampler objects bound to texture units, the
/// pixel pack and unpack buffers and parameters, polygon offset, and rasterizer
/// discard. Don't clear or invalidate the framebuffer's attachments.</para>
/// <para>See <c>mln_open_gl_custom_layer_pass</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="Framebuffer">
/// The framebuffer that the map draws into, or 0 for the default one.
/// </param>
public readonly partial record struct OpenGlCustomLayerPass(uint Framebuffer);
