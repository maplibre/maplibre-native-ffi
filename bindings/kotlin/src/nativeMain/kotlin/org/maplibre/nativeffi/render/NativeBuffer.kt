package org.maplibre.nativeffi.render

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.nativeHeap
import kotlinx.cinterop.rawValue
import kotlinx.cinterop.readBytes
import org.maplibre.nativeffi.internal.lifecycle.BorrowedResourceCore

/** Explicit off-heap byte buffer for reusable native readback and upload storage. */
@OptIn(ExperimentalForeignApi::class, ExperimentalNativeApi::class)
public actual class NativeBuffer
private constructor(private val pointer: CPointer<ByteVar>?, private val length: Long) :
  AutoCloseable {
  private val core = BorrowedResourceCore("NativeBuffer") { releaseNative() }
  @Suppress("unused")
  private val cleaner: Cleaner = createCleaner(core) { it.releaseNativeForCleaner() }

  public actual fun byteLength(): Long = withOpenBuffer { length }

  public actual fun toByteArray(): ByteArray = withOpenBuffer {
    if (pointer == null || length == 0L) ByteArray(0) else pointer.readBytes(length.toInt())
  }

  internal fun <T> borrow(block: (CPointer<ByteVar>?, Long) -> T): T = withOpenBuffer {
    block(pointer, length)
  }

  internal fun ensureCapacity(requiredBytes: ULong) {
    withOpenBuffer {
      require(requiredBytes <= Long.MAX_VALUE.toULong()) { "required byte length is too large" }
      require(length >= requiredBytes.toLong()) { "buffer is smaller than required byte length" }
    }
  }

  public actual override fun close() = core.close()

  private fun <T> withOpenBuffer(block: () -> T): T = core.withOpenResource(block)

  private fun releaseNative() {
    pointer?.let { nativeHeap.free(it.rawValue) }
  }

  public actual companion object {
    public actual fun allocate(byteLength: Long): NativeBuffer {
      require(byteLength >= 0) { "byteLength must be non-negative" }
      require(byteLength <= Int.MAX_VALUE) { "byteLength exceeds Kotlin/Native allocation limit" }
      val pointer =
        if (byteLength == 0L) null else nativeHeap.allocArray<ByteVar>(byteLength.toInt())
      return NativeBuffer(pointer, byteLength)
    }
  }
}
