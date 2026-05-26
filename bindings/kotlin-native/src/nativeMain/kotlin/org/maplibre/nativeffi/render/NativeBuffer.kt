package org.maplibre.nativeffi.render

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.readBytes

/** Explicit off-heap byte buffer for reusable native readback and upload storage. */
@OptIn(ExperimentalForeignApi::class)
public class NativeBuffer
private constructor(private val pointer: CPointer<ByteVar>?, private val length: Long) :
  AutoCloseable {
  private var closed = false

  public fun byteLength(): Long {
    ensureOpen()
    return length
  }

  public fun toByteArray(): ByteArray {
    ensureOpen()
    return if (pointer == null || length == 0L) ByteArray(0) else pointer.readBytes(length.toInt())
  }

  internal fun pointer(): CPointer<ByteVar>? {
    ensureOpen()
    return pointer
  }

  internal fun ensureCapacity(requiredBytes: ULong) {
    ensureOpen()
    require(requiredBytes <= Long.MAX_VALUE.toULong()) { "required byte length is too large" }
    require(length >= requiredBytes.toLong()) { "buffer is smaller than required byte length" }
  }

  private fun ensureOpen() {
    check(!closed) { "NativeBuffer is already closed" }
  }

  override fun close() {
    if (closed) return
    closed = true
    pointer?.let { nativeHeap.free(it.rawValue) }
  }

  public companion object {
    public fun allocate(byteLength: Long): NativeBuffer {
      require(byteLength >= 0) { "byteLength must be non-negative" }
      require(byteLength <= Int.MAX_VALUE) { "byteLength exceeds Kotlin/Native allocation limit" }
      val pointer =
        if (byteLength == 0L) null else nativeHeap.allocArray<ByteVar>(byteLength.toInt())
      return NativeBuffer(pointer, byteLength)
    }
  }
}
