package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.cValue
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.toCValues
import org.maplibre.nativeffi.internal.c.mln_buffer_view
import org.maplibre.nativeffi.internal.memory.toCSize

/** Materializes byte-array views and copies owned C buffers. */
@OptIn(ExperimentalForeignApi::class)
internal object ByteStructs {
  fun bufferView(value: ByteArray, scope: MemScope): CValue<mln_buffer_view> = cValue {
    data = bytePointer(value, scope)
    size = value.size.toCSize()
  }

  fun bufferViewPointer(value: ByteArray, scope: MemScope): CPointer<mln_buffer_view> {
    val native = scope.alloc<mln_buffer_view>()
    native.data = bytePointer(value, scope)
    native.size = value.size.toCSize()
    return native.ptr
  }

  fun setBufferView(native: mln_buffer_view, value: ByteArray, scope: MemScope) {
    native.data = bytePointer(value, scope)
    native.size = value.size.toCSize()
  }

  fun copyBufferView(value: mln_buffer_view): ByteArray {
    val size = checkedInt(value.size.toULong())
    return if (size == 0) ByteArray(0)
    else value.data!!.reinterpret<kotlinx.cinterop.ByteVar>().readBytes(size)
  }

  private fun bytePointer(value: ByteArray, scope: MemScope): CPointer<UByteVar>? =
    if (value.isEmpty()) null else value.toCValues().getPointer(scope).reinterpret()

  private fun checkedInt(value: ULong): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "byte buffer size exceeds Int.MAX_VALUE" }
    return value.toInt()
  }
}
