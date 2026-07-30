@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.examples.composewebmap

import kotlin.js.JsAny
import kotlin.js.Promise
import org.w3c.dom.HTMLCanvasElement

external interface MapLibreModule : JsAny {
  @JsName("_mln_browser_map_init")
  fun init(
    width: Int,
    height: Int,
    scale: Double,
    longitude: Double,
    latitude: Double,
    zoom: Double,
    bearing: Double,
    pitch: Double,
    context: Int,
    texture: Int,
  ): Int

  @JsName("_mln_browser_map_render_frame") fun renderFrame(): Int

  @JsName("_mln_browser_map_resize_borrowed")
  fun resize(width: Int, height: Int, scale: Double, context: Int, texture: Int): Int

  @JsName("_mln_browser_map_move_by") fun moveBy(x: Double, y: Double): Int

  @JsName("_mln_browser_map_scale_by") fun scaleBy(scale: Double, x: Double, y: Double): Int

  @JsName("_mln_browser_map_rotate_pitch_by") fun rotatePitchBy(bearing: Double, pitch: Double): Int

  @JsName("_mln_browser_map_cancel_transitions") fun cancelTransitions(): Int

  fun importWebGLContext(context: JsAny): Int

  fun importWebGLTexture(texture: JsAny): Int

  fun unregisterWebGLTexture(texture: Int)
}

@JsName("createMapLibreComposeModule")
external fun createMapLibreComposeModule(options: JsAny): Promise<MapLibreModule>

@JsFun("() => ({ locateFile: path => path, printErr: message => console.error(message) })")
external fun mapLibreModuleOptions(): JsAny

@JsFun(
  "() => { const root = document.querySelector('#composeApp'); const hosts = root ? root.querySelectorAll('*') : []; for (const host of hosts) { const canvas = host.shadowRoot?.querySelector('canvas'); if (canvas) return canvas; } return null; }"
)
external fun composeCanvas(): HTMLCanvasElement?

@JsFun("canvas => canvas.getContext('webgl2')")
external fun webGL2Context(canvas: HTMLCanvasElement): JsAny?

@JsFun(
  """(gl, width, height) => {
    const texture = gl.createTexture();
    const previous = gl.getParameter(gl.TEXTURE_BINDING_2D);
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA8, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    gl.bindTexture(gl.TEXTURE_2D, previous);
    return texture;
  }"""
)
external fun createMapTexture(context: JsAny, width: Int, height: Int): JsAny

@JsFun(
  """texture => {
    const gl = globalThis.__composeSkikoGL;
    const id = gl.getNewId(gl.textures);
    gl.textures[id] = texture;
    return id;
  }"""
)
external fun pushSkikoTexture(texture: JsAny): Int

@JsFun("() => globalThis.__composeSkiaDirectContext ?? 0")
external fun composeSkiaDirectContextPointer(): Int
