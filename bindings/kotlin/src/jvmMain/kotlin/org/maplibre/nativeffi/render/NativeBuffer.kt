package org.maplibre.nativeffi.render

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import java.lang.ref.Cleaner
import org.maplibre.nativeffi.internal.lifecycle.BorrowedResourceCore

private val nativeBufferCleaner: Cleaner = Cleaner.create()

/** Explicit off-heap byte buffer for reusable native readback and upload storage. */
public actual class NativeBuffer
private constructor(private val segment: MemorySegment, private val length: Long, arena: Arena) :
  AutoCloseable {
  private val nativeReference = NativeReference(arena)
  private val cleanable = nativeBufferCleaner.register(this, nativeReference)
  private val core = BorrowedResourceCore("NativeBuffer") { releaseNative() }

  public actual fun byteLength(): Long = withOpenBuffer { length }

  public actual fun toByteArray(): ByteArray = withOpenBuffer {
    if (length == 0L) ByteArray(0) else segment.toArray(ValueLayout.JAVA_BYTE)
  }

  internal fun <T> borrow(block: (MemorySegment, Long) -> T): T = withOpenBuffer {
    block(segment, length)
  }

  internal fun ensureCapacity(requiredBytes: Long) {
    withOpenBuffer {
      require(length >= requiredBytes) { "buffer is smaller than required byte length" }
    }
  }

  public actual override fun close() = core.close()

  private fun <T> withOpenBuffer(block: () -> T): T = core.withOpenResource(block)

  private fun releaseNative() {
    nativeReference.close()
    cleanable.clean()
  }

  public actual companion object {
    public actual fun allocate(byteLength: Long): NativeBuffer {
      require(byteLength >= 0) { "byteLength must be non-negative" }
      val arena = Arena.ofShared()
      val segment = if (byteLength == 0L) MemorySegment.NULL else arena.allocate(byteLength)
      return NativeBuffer(segment, byteLength, arena)
    }
  }

  private class NativeReference(private val arena: Arena) : Runnable {
    @Volatile private var closed = false

    @Synchronized
    fun close() {
      if (!closed) {
        closed = true
        arena.close()
      }
    }

    override fun run() {
      close()
    }
  }
}
