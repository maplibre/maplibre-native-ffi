@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.examples.composewebmap

import kotlin.js.JsAny
import kotlin.js.Promise
import org.w3c.dom.HTMLCanvasElement

external interface EmscriptenWebGpu : JsAny {
  fun importJsDevice(device: JsAny): Int

  fun importJsTexture(texture: JsAny, device: Int): Int

  fun importJsTextureView(textureView: JsAny, texture: Int): Int

  fun getJsObject(pointer: Int): JsAny
}

external interface MapLibreModule : JsAny {
  val webgpu: EmscriptenWebGpu

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
    device: Int,
    queue: Int,
  ): Int

  @JsName("_mln_browser_map_init_borrowed")
  fun initBorrowed(
    width: Int,
    height: Int,
    scale: Double,
    longitude: Double,
    latitude: Double,
    zoom: Double,
    bearing: Double,
    pitch: Double,
    device: Int,
    queue: Int,
    texture: Int,
    textureView: Int,
    textureFormat: Int,
  ): Int

  @JsName("_mln_browser_map_set_borrowed_texture_target")
  fun setBorrowedTarget(texture: Int, textureView: Int, textureFormat: Int): Int

  @JsName("_mln_browser_map_clear_borrowed_texture_target") fun clearBorrowedTarget(): Int

  @JsName("_mln_browser_map_release_borrowed_texture_import")
  fun releaseBorrowedTargetImport(texture: Int, textureView: Int)

  @JsName("_mln_browser_map_webgpu_texture_format") fun webGpuTextureFormat(format: Int): Int

  @JsName("_mln_browser_map_render_frame") fun renderFrame(): Int

  @JsName("_mln_browser_map_acquire_owned_texture") fun acquireOwnedTexture(): Int

  @JsName("_mln_browser_map_release_owned_texture_frame") fun releaseOwnedTexture(): Int

  @JsName("_mln_browser_map_resize") fun resize(width: Int, height: Int, scale: Double): Int

  @JsName("_mln_browser_map_move_by") fun moveBy(x: Double, y: Double): Int

  @JsName("_mln_browser_map_scale_by") fun scaleBy(scale: Double, x: Double, y: Double): Int

  @JsName("_mln_browser_map_rotate_pitch_by") fun rotatePitchBy(bearing: Double, pitch: Double): Int

  @JsName("_mln_browser_map_cancel_transitions") fun cancelTransitions(): Int

  @JsName("_mln_browser_map_reset_orientation") fun resetOrientation(): Int
}

@JsName("createMapLibreModule")
external fun createMapLibreModule(options: JsAny): Promise<MapLibreModule>

external interface WebGpuStagingBridge : JsAny {
  val devicePointer: Int
  val gpuCopies: Int
  val cpuReadbacks: Int
  val inFlight: Boolean
  val ready: Boolean
  val adapterInfo: JsAny

  fun resize(width: Int, height: Int)

  fun borrowTarget(): BorrowedWebGpuTarget

  fun finishTarget(present: Boolean)

  fun uploadToWebGl(webGl: JsAny, texture: JsAny): Boolean
}

external interface BorrowedWebGpuTarget : JsAny {
  val texturePointer: Int
  val textureViewPointer: Int
  val textureFormat: Int
}

@JsFun("() => ({ locateFile: path => path, printErr: message => console.error(message) })")
external fun mapLibreModuleOptions(): JsAny

@JsFun(
  """() => {
    const findCanvas = root => {
      const direct = root.querySelector("canvas");
      if (direct) return direct;
      for (const element of root.querySelectorAll("*")) {
        if (element.shadowRoot) {
          const nested = findCanvas(element.shadowRoot);
          if (nested) return nested;
        }
      }
      return null;
    };
    return findCanvas(document.querySelector("#composeApp"));
  }"""
)
external fun composeCanvas(): HTMLCanvasElement?

@JsFun("canvas => canvas.getContext('webgl2')")
external fun webGl2Context(canvas: HTMLCanvasElement): JsAny?

@JsFun(
  """async module => {
    if (!navigator.gpu) {
      throw new Error("WebGPU is unavailable");
    }
    const adapter = await navigator.gpu.requestAdapter();
    if (!adapter) {
      throw new Error("No WebGPU adapter is available");
    }
    const device = await adapter.requestDevice();
    device.addEventListener("uncapturederror", event => {
      console.error("WebGPU uncaptured error:", event.error);
    });
    device.lost.then(info => {
      console.error("WebGPU device lost: " + info.reason + ": " + info.message);
    });

    const canvas = new OffscreenCanvas(1, 1);
    const context = canvas.getContext("2d", {
      alpha: true,
      willReadFrequently: false,
    });
    if (!context) {
      throw new Error("OffscreenCanvas 2D context is unavailable");
    }
    if (
      typeof context.transferToGPUTexture !== "function" ||
      typeof context.transferBackFromGPUTexture !== "function"
    ) {
      throw new Error(
        "Canvas2D WebGPU transfer is unavailable. " +
        "Enable chrome://flags/#enable-experimental-web-platform-features.",
      );
    }

    // Prime the Canvas2D resource provider so later map-frame borrows can
    // require a truly shared (zero-copy) WebGPU texture.
    const warmupTexture = context.transferToGPUTexture({
      device,
      usage: GPUTextureUsage.RENDER_ATTACHMENT,
      requireZeroCopy: false,
      label: "Compose MapLibre Canvas2D bridge warmup",
    });
    const bridgeFormat = warmupTexture.format;
    context.transferBackFromGPUTexture();
    const textureFormat =
      bridgeFormat === "rgba8unorm"
        ? module._mln_browser_map_webgpu_texture_format(0)
        : bridgeFormat === "bgra8unorm"
          ? module._mln_browser_map_webgpu_texture_format(1)
          : 0;
    if (!textureFormat) {
      throw new Error("Unsupported Canvas2D WebGPU format: " + bridgeFormat);
    }
    const adapterInfo = adapter.info ??
      (adapter.requestAdapterInfo ? await adapter.requestAdapterInfo() : {});

    const bridge = {
      canvas,
      module,
      devicePointer: module.webgpu.importJsDevice(device),
      adapterInfo,
      gpuCopies: 0,
      cpuReadbacks: 0,
      width: 1,
      height: 1,
      inFlight: false,
      ready: false,
      borrowedTexture: null,
      texturePointer: 0,
      textureViewPointer: 0,
      resize(width, height) {
        if (this.inFlight) {
          context.transferBackFromGPUTexture();
          module._mln_browser_map_release_borrowed_texture_import(
            this.texturePointer,
            this.textureViewPointer,
          );
          this.borrowedTexture = null;
          this.texturePointer = 0;
          this.textureViewPointer = 0;
        }
        this.width = width;
        this.height = height;
        this.inFlight = false;
        this.ready = false;
        canvas.width = width;
        canvas.height = height;
      },
      borrowTarget() {
        if (this.inFlight || this.ready) {
          throw new Error("A WebGPU canvas transfer is already pending");
        }
        const destination = context.transferToGPUTexture({
          device,
          usage: GPUTextureUsage.RENDER_ATTACHMENT,
          requireZeroCopy: true,
          label: "Compose MapLibre Canvas2D bridge frame",
        });
        if (destination.format !== bridgeFormat) {
          context.transferBackFromGPUTexture();
          throw new Error(
            "Canvas2D GPU texture format changed from " +
            bridgeFormat + " to " + destination.format,
          );
        }
        const texturePointer = module.webgpu.importJsTexture(
          destination,
          this.devicePointer,
        );
        const view = destination.createView({
          label: "Compose MapLibre borrowed target view",
        });
        const textureViewPointer = module.webgpu.importJsTextureView(
          view,
          texturePointer,
        );
        this.borrowedTexture = destination;
        this.texturePointer = texturePointer;
        this.textureViewPointer = textureViewPointer;
        this.inFlight = true;
        return { texturePointer, textureViewPointer, textureFormat };
      },
      finishTarget(present) {
        if (!this.inFlight) {
          throw new Error("No Canvas2D WebGPU target is borrowed");
        }
        context.transferBackFromGPUTexture();
        module._mln_browser_map_release_borrowed_texture_import(
          this.texturePointer,
          this.textureViewPointer,
        );
        this.borrowedTexture = null;
        this.texturePointer = 0;
        this.textureViewPointer = 0;
        this.inFlight = false;
        this.ready = present;
      },
      uploadToWebGl(gl, texture) {
        if (!this.ready) {
          return false;
        }
        const previousTexture = gl.getParameter(gl.TEXTURE_BINDING_2D);
        const previousFlipY = gl.getParameter(gl.UNPACK_FLIP_Y_WEBGL);
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false);
        gl.texSubImage2D(
          gl.TEXTURE_2D,
          0,
          0,
          0,
          gl.RGBA,
          gl.UNSIGNED_BYTE,
          canvas,
        );
        gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, previousFlipY);
        gl.bindTexture(gl.TEXTURE_2D, previousTexture);
        this.ready = false;
        this.gpuCopies++;
        return true;
      },
    };
    globalThis.__mapLibreComposeBridge = bridge;
    return bridge;
  }"""
)
external fun createWebGpuStagingBridge(module: MapLibreModule): Promise<WebGpuStagingBridge>

@JsFun(
  """(gl, width, height) => {
    const texture = gl.createTexture();
    if (!texture) {
      throw new Error("Unable to create Compose WebGL map texture");
    }
    const previous = gl.getParameter(gl.TEXTURE_BINDING_2D);
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texStorage2D(gl.TEXTURE_2D, 1, gl.RGBA8, width, height);
    gl.bindTexture(gl.TEXTURE_2D, previous);
    return texture;
  }"""
)
external fun createComposeMapTexture(webGl: JsAny, width: Int, height: Int): JsAny

@JsFun(
  """(gl, texture) => {
    const textureHandle = gl.getNewId(gl.textures);
    gl.textures[textureHandle] = texture;
    return textureHandle;
  }"""
)
external fun pushSkikoTexture(gl: JsAny, texture: JsAny): Int

@JsFun(
  "() => globalThis.__mapLibreComposeFramePumps = " +
    "(globalThis.__mapLibreComposeFramePumps ?? 0) + 1"
)
external fun recordComposeFramePump()

@JsFun(
  "() => globalThis.__mapLibreComposeRenders = " + "(globalThis.__mapLibreComposeRenders ?? 0) + 1"
)
external fun recordComposeRender()
