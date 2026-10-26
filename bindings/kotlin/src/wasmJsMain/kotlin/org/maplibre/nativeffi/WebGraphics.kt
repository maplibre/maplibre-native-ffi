@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.unsignedWord
import org.maplibre.nativeffi.internal.c.word
import org.maplibre.nativeffi.internal.lifecycle.ViewScope
import org.maplibre.nativeffi.internal.loader.ensureNativeLibrary

/**
 * Browser-only graphics interop entry points.
 *
 * Render target descriptors name WebGPU objects and WebGL contexts by handles in the browser
 * module's own tables, and only the module can create those handles. These functions import the
 * page's own objects into the module. An import is a copy, not a move: its handle holds one
 * reference of the module's, and closing the import releases only that reference, so the JavaScript
 * object stays usable.
 *
 * Close an import only after every session that names it has completed its detach or been
 * destroyed, as the descriptors' borrowed handles require. An import has no finalizer, so an import
 * that is never closed keeps its JavaScript object alive.
 *
 * A module carries one backend. The WebGPU functions need a WebGPU module, and
 * [registerWebGlContext] needs a WebGL module. Import from the agent that runs Kotlin, which is the
 * agent that drives the sessions.
 */
public object WebGraphics {
  /**
   * Imports a `GPUDevice` as the `WGPUDevice` of a `WebGpuContextDescriptor`. Pass a null instance
   * and a null queue beside it: sessions use neither, and null names the device's default queue.
   *
   * Throws an `IllegalArgumentException` when [device] is not a `GPUDevice` of this realm.
   */
  public fun importWebGpuDevice(device: JsAny): ImportedWebGpuObject =
    importWebGpu(
      "device",
      "GPUDevice",
      import = { mlnKwWebGpuImportDevice(device) },
      release = { mlnKwWebGpuReleaseDevice(it) },
    )

  /**
   * Imports a `GPUCanvasContext`, the `getContext("webgpu")` of a canvas element or an
   * `OffscreenCanvas`, as the `WGPUSurface` of a `WebGpuSurfaceDescriptor`.
   *
   * Throws an `IllegalArgumentException` when [canvasContext] is not a `GPUCanvasContext` of this
   * realm.
   */
  public fun importWebGpuSurface(canvasContext: JsAny): ImportedWebGpuObject =
    importWebGpu(
      "canvasContext",
      "GPUCanvasContext",
      import = { mlnKwWebGpuImportSurface(canvasContext) },
      release = { mlnKwWebGpuReleaseSurface(it) },
    )

  /**
   * Imports a `GPUTexture` as a `WGPUTexture`, such as a texture of a borrowed texture ring.
   *
   * Throws an `IllegalArgumentException` when [texture] is not a `GPUTexture` of this realm.
   */
  public fun importWebGpuTexture(texture: JsAny): ImportedWebGpuObject =
    importWebGpu(
      "texture",
      "GPUTexture",
      import = { mlnKwWebGpuImportTexture(texture) },
      release = { mlnKwWebGpuReleaseTexture(it) },
    )

  /**
   * Imports a `GPUTextureView` as a `WGPUTextureView`, such as a view of a borrowed texture ring.
   *
   * Throws an `IllegalArgumentException` when [view] is not a `GPUTextureView` of this realm.
   */
  public fun importWebGpuTextureView(view: JsAny): ImportedWebGpuObject =
    importWebGpu(
      "view",
      "GPUTextureView",
      import = { mlnKwWebGpuImportTextureView(view) },
      release = { mlnKwWebGpuReleaseTextureView(it) },
    )

  /**
   * Returns the `WGPUTextureFormat` value of a `GPUTextureFormat` name, such as the
   * `navigator.gpu.getPreferredCanvasFormat()` that a surface descriptor's format takes.
   *
   * Throws an `IllegalArgumentException` when the module knows no format of that name.
   */
  public fun webGpuTextureFormat(format: String): UInt {
    requireWebGpu()
    val value = mlnKwWebGpuTextureFormat(format)
    require(value >= 0) { "unknown WebGPU texture format: $format" }
    return value.toUInt()
  }

  /**
   * Returns the JavaScript object behind a WebGPU handle of the module, such as the `GPUTexture` of
   * an acquired frame's `WebGpuTextureFrame.texture`. The object is valid for as long as the handle
   * is, so a frame's texture is valid until the frame's release.
   *
   * Throws an `IllegalArgumentException` when [pointer] is null or names no object of the module.
   */
  public fun webGpuObject(pointer: NativePointer): JsAny {
    requireWebGpu()
    require(!pointer.isNull) { "pointer is null" }
    return requireNotNull(mlnKwWebGpuObject(word(pointer.address))) {
      "pointer names no WebGPU object of this module"
    }
  }

  /**
   * Registers a `WebGL2RenderingContext` as the context handle of a `WebGlContextDescriptor` of
   * kind `EXISTING`. The registration records the attributes that the context was created with, and
   * the C reference's rules for an existing context apply: MapLibre draws with a depth and a
   * stencil buffer.
   *
   * Throws an `IllegalArgumentException` when [context] is not a `WebGL2RenderingContext` of this
   * realm.
   */
  public fun registerWebGlContext(context: JsAny): RegisteredWebGlContext {
    ensureNativeLibrary()
    if (!mlnKwHasWebGl()) {
      throw MaplibreException(Status.UNSUPPORTED, "this browser module carries no WebGL backend")
    }
    val handle = mlnKwWebGlRegisterContext(context)
    require(handle != 0) { "context is not a WebGL2RenderingContext of this realm" }
    return RegisteredWebGlContext(handle)
  }

  private fun importWebGpu(
    name: String,
    type: String,
    import: () -> Int,
    release: (Int) -> Unit,
  ): ImportedWebGpuObject {
    requireWebGpu()
    val handle = import()
    require(handle != 0) { "$name is not a $type of this realm" }
    return ImportedWebGpuObject(unsignedWord(handle), release)
  }

  private fun requireWebGpu() {
    ensureNativeLibrary()
    if (!mlnKwHasWebGpu()) {
      throw MaplibreException(Status.UNSUPPORTED, "this browser module carries no WebGPU backend")
    }
  }
}

/**
 * A WebGPU object of the page, imported into the browser module by [WebGraphics]. Close it only
 * after every session that names [pointer] has completed its detach or been destroyed: [close]
 * releases the module's reference at once, and the JavaScript object stays usable.
 */
public class ImportedWebGpuObject
internal constructor(private val address: Long, private val release: (Int) -> Unit) :
  AutoCloseable {
  private val scope = ViewScope()
  private var closed = false

  /**
   * The module's handle to the object, for a render target descriptor. Throws an
   * `IllegalStateException` after [close], and a pointer read before then stops working too.
   */
  public val pointer: NativePointer
    get() {
      check(!closed) { "ImportedWebGpuObject is closed" }
      return NativePointer.scoped(address, scope)
    }

  /** Releases the module's reference. A second call does nothing. */
  override fun close() {
    if (closed) return
    closed = true
    scope.close()
    release(word(address))
  }
}

/**
 * A WebGL 2 context of the page, registered with the browser module by [WebGraphics]. Close it only
 * after every session that names [context] has completed its detach or been destroyed: [close]
 * removes the registration at once, and the JavaScript context stays usable.
 */
public class RegisteredWebGlContext internal constructor(private val handle: Int) : AutoCloseable {
  private var closed = false

  /**
   * The context's `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for a `WebGlContextDescriptor` of kind
   * `EXISTING`. Throws an `IllegalStateException` after [close].
   */
  public val context: Int
    get() {
      check(!closed) { "RegisteredWebGlContext is closed" }
      return handle
    }

  /**
   * Removes the registration. No later registration reuses the handle, so a descriptor that still
   * names it fails to make a context current. A second call does nothing.
   */
  override fun close() {
    if (closed) return
    closed = true
    mlnKwWebGlUnregisterContext(handle)
  }
}

// The graphics libraries of bindings/kotlin/wasm, which a module of each backend exports as a
// property of its own.
private fun mlnKwHasWebGpu(): Boolean =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu !== undefined")

private fun mlnKwHasWebGl(): Boolean =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGl !== undefined")

private fun mlnKwWebGpuImportDevice(device: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.importDevice(device)")

private fun mlnKwWebGpuImportSurface(context: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.importSurface(context)")

private fun mlnKwWebGpuImportTexture(texture: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.importTexture(texture)")

private fun mlnKwWebGpuImportTextureView(view: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.importTextureView(view)")

private fun mlnKwWebGpuReleaseDevice(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.releaseDevice(handle)")

private fun mlnKwWebGpuReleaseSurface(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.releaseSurface(handle)")

private fun mlnKwWebGpuReleaseTexture(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.releaseTexture(handle)")

private fun mlnKwWebGpuReleaseTextureView(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.releaseTextureView(handle)")

private fun mlnKwWebGpuObject(handle: Int): JsAny? =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.object(handle)")

private fun mlnKwWebGpuTextureFormat(name: String): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGpu.textureFormat(name)")

private fun mlnKwWebGlRegisterContext(context: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGl.registerContext(context)")

private fun mlnKwWebGlUnregisterContext(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebGl.unregisterContext(handle)")
