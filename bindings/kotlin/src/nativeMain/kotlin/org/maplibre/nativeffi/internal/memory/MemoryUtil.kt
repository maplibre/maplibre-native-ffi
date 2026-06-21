package org.maplibre.nativeffi.internal.memory

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.cstr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.toCValues
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.internal.status.Status

/** Memory helpers for Kotlin/Native C ABI calls. */
@OptIn(ExperimentalForeignApi::class)
internal object MemoryUtil {
  /** Rejects strings that C would truncate when passed as null-terminated text. */
  fun requireValidCString(value: String) {
    if ('\u0000' in value) {
      throw Status.invalidArgument("C string inputs cannot contain embedded NUL characters")
    }
  }

  /** Allocates a null-terminated UTF-8 string for one C call. */
  fun cString(scope: MemScope, value: String): CPointer<ByteVar> {
    requireValidCString(value)
    return value.cstr.getPointer(scope)
  }

  /** Allocates UTF-8 bytes for an explicit-length C string view. */
  fun utf8Bytes(scope: MemScope, value: String): CPointer<ByteVar>? {
    val bytes = value.encodeToByteArray()
    return if (bytes.isEmpty()) null else bytes.toCValues().getPointer(scope)
  }

  /** Copies explicit-length UTF-8 bytes from C-owned storage. */
  fun copyStringView(data: CPointer<ByteVar>?, size: ULong): String {
    if (data == null || size == 0UL) return ""
    require(size <= Int.MAX_VALUE.toULong()) { "string view size exceeds Int.MAX_VALUE" }
    return data.readBytes(size.toInt()).decodeToString()
  }

  /** Copies a null-terminated UTF-8 C string from C-owned storage. */
  fun copyCString(data: CPointer<ByteVar>?): String = data?.toKString() ?: ""
}
