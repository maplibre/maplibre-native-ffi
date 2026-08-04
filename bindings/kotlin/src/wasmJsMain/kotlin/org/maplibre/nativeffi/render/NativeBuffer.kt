package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.lifecycle.BorrowedResourceCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapPointer

@JsFun("(size) => globalThis.__maplibreNativeC._malloc(size)")
private external fun allocate(size: Int): Int

@JsFun("(address) => globalThis.__maplibreNativeC._free(address)")
private external fun free(address: Int)

/**
 * Reusable readback and upload storage, held in the Emscripten heap.
 *
 * Native writes readback pixels through a pointer, so the storage has to live where native can
 * address it -- which is the module's heap, not this module's. That is also why the buffer is
 * explicit rather than a `ByteArray`: a garbage-collected Kotlin array has no address native could
 * be given, and copying one in and out per frame would cost a transfer each way.
 *
 * A browser host cannot recover leaked heap by restarting a process, and Kotlin/Wasm has no
 * finalizer to release it from, so closing this is the only thing that frees it.
 */
public actual class NativeBuffer
private constructor(private val address: Int, private val length: Long) : AutoCloseable {
  private val core = BorrowedResourceCore("NativeBuffer") { free(address) }

  public actual fun byteLength(): Long = core.withOpenResource { length }

  public actual fun toByteArray(): ByteArray = core.withOpenResource {
    // One boundary crossing regardless of size; see Heap.
    Heap.loadBytes(HeapPointer(address), length.toInt())
  }

  /** Runs [block] with the buffer's address, keeping it open for the call. */
  internal fun <T> borrow(block: (HeapPointer, Long) -> T): T = core.withOpenResource {
    block(HeapPointer(address), length)
  }

  internal fun ensureCapacity(requiredBytes: Long) {
    core.withOpenResource {
      Status.requireArgument(length >= requiredBytes) {
        "buffer is smaller than required byte length"
      }
    }
  }

  public actual override fun close(): Unit = core.close()

  public actual companion object {
    public actual fun allocate(byteLength: Long): NativeBuffer {
      Status.requireArgument(byteLength >= 0) { "byteLength must be non-negative" }
      // Pointers are 32 bits on this target, so a length native could not address is rejected here
      // rather than becoming a truncated allocation.
      Status.requireArgument(byteLength <= Int.MAX_VALUE) {
        "byteLength must fit a 32-bit pointer on this target"
      }
      BrowserModule.require()
      if (byteLength == 0L) return NativeBuffer(0, 0)
      val address = allocate(byteLength.toInt())
      if (address == 0) {
        throw Status.invalidState(
          "The MapLibre Native browser module could not allocate $byteLength bytes"
        )
      }
      return NativeBuffer(address, byteLength)
    }
  }
}
