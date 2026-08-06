package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.lifecycle.BorrowedResourceCore
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapPointer

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
  private val core =
    BorrowedResourceCore("NativeBuffer") { if (address != 0) Heap.release(HeapPointer(address)) }

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
      // rather than becoming a truncated allocation.
      Status.requireArgument(byteLength <= Int.MAX_VALUE) {
        "byteLength must fit a 32-bit pointer on this target"
      }
      // Asked of the module rather than assumed from the link settings, so this stays right if the
      // heap is linked at another size. It is a good deal tighter than the pointer bound above: the
      // module's memory is half a gigabyte by default, where a 32-bit pointer addresses four.
      val heapBytes = Heap.byteLength()
      Status.requireArgument(byteLength <= heapBytes) {
        "byteLength must not exceed the browser module's whole $heapBytes-byte heap"
      }
      if (byteLength == 0L) return NativeBuffer(0, 0)
      // The scratch allocator's own acquisition, which reports an exhausted heap the same way: a
      // caller cannot tell the two apart, and two spellings of one failure drift the first time
      // either is reworded.
      return NativeBuffer(Heap.acquire(byteLength.toInt()).address, byteLength)
    }
  }
}
