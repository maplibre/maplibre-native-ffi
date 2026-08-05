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
 * Returns the storage at [address] to the module's allocator, or retires it when the module has
 * gone.
 *
 * A buffer is not owner-affine and is deliberately not one of the handles a shutdown refuses to
 * leave open: nothing about it needs the owner thread, and the thing that would free it is the
 * heap's own allocator. A shutdown that released the module took this storage with the heap it
 * lived in, so there is nothing left to free and the close simply retires. Calling the allocator
 * there would reach a module reference that is now null and report a JavaScript type error naming
 * `_free`, from inside a close that has nothing left to fail at.
 */
private fun releaseStorage(address: Int) {
  if (!BrowserModule.isLoaded()) return
  free(address)
}

/**
 * Reusable readback and upload storage, held in the Emscripten heap.
 *
 * Native writes readback pixels through a pointer, so the storage has to live where native can
 * address it -- which is the module's heap, not this module's. That is also why the buffer is
 * explicit rather than a `ByteArray`: a garbage-collected Kotlin array has no address native could
 * be given, and copying one in and out per frame would cost a transfer each way.
 *
 * A browser host cannot recover leaked heap by restarting a process, and Kotlin/Wasm has no
 * finalizer to release it from, so closing this is the only thing that frees it. The one exception
 * is a shutdown, which releases the whole heap at once: closing afterwards succeeds and does
 * nothing, and reading the bytes reports that they went with it.
 */
public actual class NativeBuffer
private constructor(private val address: Int, private val length: Long) : AutoCloseable {
  private val core = BorrowedResourceCore("NativeBuffer") { releaseStorage(address) }

  public actual fun byteLength(): Long = core.withOpenResource { length }

  public actual fun toByteArray(): ByteArray = core.withOpenResource {
    // Asked before the copy, and the one place this differs from closing: a shutdown released the
    // heap these bytes lived in, so there is nothing to free but there is also nothing to read.
    // Without this the copy below would reach a module reference that is now null and report a
    // JavaScript type error naming `HEAPU8` rather than the binding failure that says what
    // happened.
    BrowserModule.require()
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
