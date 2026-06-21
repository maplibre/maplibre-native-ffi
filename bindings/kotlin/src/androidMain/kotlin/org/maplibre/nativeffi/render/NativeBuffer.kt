package org.maplibre.nativeffi.render

import java.nio.ByteBuffer
import org.maplibre.nativeffi.internal.lifecycle.BorrowedResourceCore

/** Explicit direct byte buffer for reusable Android JNI readback and upload storage. */
public actual class NativeBuffer
private constructor(private val buffer: ByteBuffer, private val length: Long) : AutoCloseable {
  private val core = BorrowedResourceCore("NativeBuffer") {}

  public actual fun byteLength(): Long = withOpenBuffer { length }

  public actual fun toByteArray(): ByteArray = withOpenBuffer {
    val bytes = ByteArray(length.toInt())
    val duplicate = buffer.asReadOnlyBuffer()
    duplicate.position(0)
    duplicate.get(bytes)
    bytes
  }

  internal fun borrowBuffer(): ByteBuffer = withOpenBuffer {
    val duplicate = buffer.duplicate()
    duplicate.position(0)
    duplicate
  }

  internal fun ensureCapacity(requiredBytes: Long) {
    withOpenBuffer {
      require(length >= requiredBytes) { "buffer is smaller than required byte length" }
    }
  }

  public actual override fun close() = core.close()

  private fun <T> withOpenBuffer(block: () -> T): T = core.withOpenResource(block)

  public actual companion object {
    public actual fun allocate(byteLength: Long): NativeBuffer {
      require(byteLength >= 0) { "byteLength must be non-negative" }
      require(byteLength <= Int.MAX_VALUE) { "byteLength must be at most Integer.MAX_VALUE" }
      return NativeBuffer(ByteBuffer.allocateDirect(byteLength.toInt()), byteLength)
    }
  }
}
