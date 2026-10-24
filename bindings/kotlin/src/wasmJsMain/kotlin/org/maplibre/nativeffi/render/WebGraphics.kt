@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.Status
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
 * [registerWebglContext] needs a WebGL module. Import from the agent that runs Kotlin, which is the
 * agent that drives the sessions.
 */
public object WebGraphics {
  /**
   * Imports a `GPUDevice` as the `WGPUDevice` of a `WebgpuContextDescriptor`. Pass a null instance
   * and a null queue beside it: sessions use neither, and null names the device's default queue.
   *
   * Throws an `IllegalArgumentException` when [device] is not a `GPUDevice` of this realm.
   */
  public fun importWebgpuDevice(device: JsAny): ImportedWebgpuObject =
    importWebgpu(
      "device",
      "GPUDevice",
      import = { mlnKwWebgpuImportDevice(device) },
      release = { mlnKwWebgpuReleaseDevice(it) },
    )

  /**
   * Imports a `GPUCanvasContext`, the `getContext("webgpu")` of a canvas element or an
   * `OffscreenCanvas`, as the `WGPUSurface` of a `WebgpuSurfaceDescriptor`.
   *
   * Throws an `IllegalArgumentException` when [canvasContext] is not a `GPUCanvasContext` of this
   * realm.
   */
  public fun importWebgpuSurface(canvasContext: JsAny): ImportedWebgpuObject =
    importWebgpu(
      "canvasContext",
      "GPUCanvasContext",
      import = { mlnKwWebgpuImportSurface(canvasContext) },
      release = { mlnKwWebgpuReleaseSurface(it) },
    )

  /**
   * Imports a `GPUTexture` as a `WGPUTexture`, such as a texture of a borrowed texture ring.
   *
   * Throws an `IllegalArgumentException` when [texture] is not a `GPUTexture` of this realm.
   */
  public fun importWebgpuTexture(texture: JsAny): ImportedWebgpuObject =
    importWebgpu(
      "texture",
      "GPUTexture",
      import = { mlnKwWebgpuImportTexture(texture) },
      release = { mlnKwWebgpuReleaseTexture(it) },
    )

  /**
   * Imports a `GPUTextureView` as a `WGPUTextureView`, such as a view of a borrowed texture ring.
   *
   * Throws an `IllegalArgumentException` when [view] is not a `GPUTextureView` of this realm.
   */
  public fun importWebgpuTextureView(view: JsAny): ImportedWebgpuObject =
    importWebgpu(
      "view",
      "GPUTextureView",
      import = { mlnKwWebgpuImportTextureView(view) },
      release = { mlnKwWebgpuReleaseTextureView(it) },
    )

  /**
   * Returns the `WGPUTextureFormat` value of a `GPUTextureFormat` name, such as the
   * `navigator.gpu.getPreferredCanvasFormat()` that a surface descriptor's format takes.
   *
   * Throws an `IllegalArgumentException` when the module knows no format of that name.
   */
  public fun webgpuTextureFormat(format: String): UInt {
    requireWebgpu()
    val value = mlnKwWebgpuTextureFormat(format)
    require(value >= 0) { "unknown WebGPU texture format: $format" }
    return value.toUInt()
  }

  /**
   * Returns the JavaScript object behind a WebGPU handle of the module, such as the `GPUTexture` of
   * an acquired frame's `WebgpuTextureFrame.texture`. The object is valid for as long as the handle
   * is, so a frame's texture is valid until the frame's release.
   *
   * Throws an `IllegalArgumentException` when [pointer] is null or names no object of the module.
   */
  public fun webgpuObject(pointer: NativePointer): JsAny {
    requireWebgpu()
    require(!pointer.isNull) { "pointer is null" }
    return requireNotNull(mlnKwWebgpuObject(word(pointer.address))) {
      "pointer names no WebGPU object of this module"
    }
  }

  /**
   * Registers a `WebGL2RenderingContext` as the context handle of a `WebglContextDescriptor` of
   * kind `EXISTING`. The registration records the attributes that the context was created with, and
   * the C reference's rules for an existing context apply: MapLibre draws with a depth and a
   * stencil buffer.
   *
   * Throws an `IllegalArgumentException` when [context] is not a `WebGL2RenderingContext` of this
   * realm.
   */
  public fun registerWebglContext(context: JsAny): RegisteredWebglContext {
    ensureNativeLibrary()
    if (!mlnKwHasWebgl()) {
      throw MaplibreException(Status.UNSUPPORTED, "this browser module carries no WebGL backend")
    }
    val handle = mlnKwWebglRegisterContext(context)
    require(handle != 0) { "context is not a WebGL2RenderingContext of this realm" }
    return RegisteredWebglContext(handle)
  }

  private fun importWebgpu(
    name: String,
    type: String,
    import: () -> Int,
    release: (Int) -> Unit,
  ): ImportedWebgpuObject {
    requireWebgpu()
    val handle = import()
    require(handle != 0) { "$name is not a $type of this realm" }
    return ImportedWebgpuObject(unsignedWord(handle), release)
  }

  private fun requireWebgpu() {
    ensureNativeLibrary()
    if (!mlnKwHasWebgpu()) {
      throw MaplibreException(Status.UNSUPPORTED, "this browser module carries no WebGPU backend")
    }
  }
}

/**
 * A WebGPU object of the page, imported into the browser module by [WebGraphics]. Close it only
 * after every session that names [pointer] has completed its detach or been destroyed: [close]
 * releases the module's reference at once, and the JavaScript object stays usable.
 */
public class ImportedWebgpuObject
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
      check(!closed) { "ImportedWebgpuObject is closed" }
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
public class RegisteredWebglContext internal constructor(private val handle: Int) : AutoCloseable {
  private var closed = false

  /**
   * The context's `EMSCRIPTEN_WEBGL_CONTEXT_HANDLE`, for a `WebglContextDescriptor` of kind
   * `EXISTING`. Throws an `IllegalStateException` after [close].
   */
  public val context: Int
    get() {
      check(!closed) { "RegisteredWebglContext is closed" }
      return handle
    }

  /**
   * Removes the registration. No later registration reuses the handle, so a descriptor that still
   * names it fails to make a context current. A second call does nothing.
   */
  override fun close() {
    if (closed) return
    closed = true
    mlnKwWebglUnregisterContext(handle)
  }
}

// The graphics libraries of bindings/kotlin/wasm, which a module of each backend exports as a
// property of its own.
private fun mlnKwHasWebgpu(): Boolean =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu !== undefined")

private fun mlnKwHasWebgl(): Boolean =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgl !== undefined")

private fun mlnKwWebgpuImportDevice(device: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.importDevice(device)")

private fun mlnKwWebgpuImportSurface(context: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.importSurface(context)")

private fun mlnKwWebgpuImportTexture(texture: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.importTexture(texture)")

private fun mlnKwWebgpuImportTextureView(view: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.importTextureView(view)")

private fun mlnKwWebgpuReleaseDevice(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.releaseDevice(handle)")

private fun mlnKwWebgpuReleaseSurface(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.releaseSurface(handle)")

private fun mlnKwWebgpuReleaseTexture(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.releaseTexture(handle)")

private fun mlnKwWebgpuReleaseTextureView(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.releaseTextureView(handle)")

private fun mlnKwWebgpuObject(handle: Int): JsAny? =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.object(handle)")

private fun mlnKwWebgpuTextureFormat(name: String): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgpu.textureFormat(name)")

private fun mlnKwWebglRegisterContext(context: JsAny): Int =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgl.registerContext(context)")

private fun mlnKwWebglUnregisterContext(handle: Int): Unit =
  js("globalThis[Symbol.for('org.maplibre.nativeffi.c')].mlnKwWebgl.unregisterContext(handle)")
