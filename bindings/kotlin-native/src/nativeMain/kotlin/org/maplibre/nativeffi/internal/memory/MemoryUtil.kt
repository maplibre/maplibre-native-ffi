package org.maplibre.nativeffi.internal.memory

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.cstr

/** Memory helpers for Kotlin/Native C ABI calls. */
@OptIn(ExperimentalForeignApi::class)
internal object MemoryUtil {
  /** Rejects strings that C would truncate when passed as null-terminated text. */
  fun requireValidCString(value: String) {
    require('\u0000' !in value) { "C string inputs cannot contain embedded NUL characters" }
  }

  /** Allocates a null-terminated UTF-8 string for one C call. */
  fun cString(scope: MemScope, value: String): CPointer<ByteVar> {
    requireValidCString(value)
    return value.cstr.getPointer(scope)
  }
}
