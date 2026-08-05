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
    /**
     * Takes [byteLength] bytes of the module's heap, or says why it could not.
     *
     * Two of the three ways this fails are the caller's, and they are separated because the remedy
     * differs. A negative length, or one no 32-bit pointer could address, is a wrong argument. A
     * length past the module's whole linear memory is also a wrong argument even though it looks
     * like a shortage: the heap is fixed at link time, so nothing a host frees would ever make that
     * request succeed. Only the third is a state failure — the heap is real but what is left of it
     * is not enough — and that one is the allocator's to report.
     *
     * It does report it, rather than taking the page's module down, because this build links with
     * `-sABORTING_MALLOC=0`. Emscripten's default is to abort on an allocation the heap cannot
     * serve, which would make the check below unreachable and leave a host with no error at all.
     */
    public actual fun allocate(byteLength: Long): NativeBuffer {
      Status.requireArgument(byteLength >= 0) { "byteLength must be non-negative" }
      // Pointers are 32 bits on this target, so a length native could not address is rejected here
      // rather than becoming a truncated allocation. Asked before the module is, because a length
      // this wrong is wrong whether or not a host has loaded anything.
      Status.requireArgument(byteLength <= Int.MAX_VALUE) {
        "byteLength must fit a 32-bit pointer on this target"
      }
      BrowserModule.require()
      // Asked of the module rather than assumed from the link settings, so this stays right if the
      // heap is linked at another size. It is a good deal tighter than the pointer bound above: the
      // module's memory is half a gigabyte by default, where a 32-bit pointer addresses four.
      val heapBytes = Heap.byteLength()
      Status.requireArgument(byteLength <= heapBytes) {
        "byteLength must not exceed the browser module's whole $heapBytes-byte heap"
      }
      if (byteLength == 0L) return NativeBuffer(0, 0)
      val address = allocate(byteLength.toInt())
      // Shared with the scratch allocator's, because a caller cannot tell the two apart and two
      // spellings of one failure drift the first time either is reworded.
      if (address == 0) throw Heap.allocationFailure(byteLength.toInt())
      return NativeBuffer(address, byteLength)
    }
  }
}
