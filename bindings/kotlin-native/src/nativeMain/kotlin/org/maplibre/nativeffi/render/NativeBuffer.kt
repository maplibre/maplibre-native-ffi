package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
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

/** Explicit off-heap byte buffer for reusable native readback and upload storage. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
public class NativeBuffer
private constructor(private val pointer: CPointer<ByteVar>?, private val length: Long) :
  AutoCloseable {
  private val state = AtomicInt(0)
  private val nativeReference = NativeReference(pointer)
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(nativeReference) { it.release() }

  public fun byteLength(): Long = withOpenBuffer { length }

  public fun toByteArray(): ByteArray = withOpenBuffer {
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

  override fun close() {
    while (true) {
      val current = state.load()
      if (current < 0) return
      if (state.compareAndSet(current, CLOSED_FLAG or current)) {
        if (current == 0) {
          nativeReference.release()
        }
        return
      }
    }
  }

  private inline fun <T> withOpenBuffer(block: () -> T): T {
    retain()
    try {
      return block()
    } finally {
      release()
    }
  }

  private fun retain() {
    while (true) {
      val current = state.load()
      check(current >= 0) { "NativeBuffer is already closed" }
      check(current < ACTIVE_MASK) { "too many active NativeBuffer borrows" }
      if (state.compareAndSet(current, current + 1)) return
    }
  }

  private fun release() {
    while (true) {
      val current = state.load()
      val active = current and ACTIVE_MASK
      check(active > 0) { "NativeBuffer borrow count underflow" }
      val next = current - 1
      if (state.compareAndSet(current, next)) {
        if (next == CLOSED_FLAG) {
          nativeReference.release()
        }
        return
      }
    }
  }

  public companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val ACTIVE_MASK = Int.MAX_VALUE

    public fun allocate(byteLength: Long): NativeBuffer {
      require(byteLength >= 0) { "byteLength must be non-negative" }
      require(byteLength <= Int.MAX_VALUE) { "byteLength exceeds Kotlin/Native allocation limit" }
      val pointer =
        if (byteLength == 0L) null else nativeHeap.allocArray<ByteVar>(byteLength.toInt())
      return NativeBuffer(pointer, byteLength)
    }
  }

  private class NativeReference(private val pointer: CPointer<ByteVar>?) {
    private val released = AtomicInt(0)

    fun release() {
      if (released.compareAndSet(0, 1)) {
        pointer?.let { nativeHeap.free(it.rawValue) }
      }
    }
  }
}
