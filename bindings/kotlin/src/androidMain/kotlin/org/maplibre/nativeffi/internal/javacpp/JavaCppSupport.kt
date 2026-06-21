package org.maplibre.nativeffi.internal.javacpp

import java.nio.charset.StandardCharsets
import org.bytedeco.javacpp.BytePointer
import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.internal.status.Status

/** Small helpers for adapting Kotlin Android code to JavaCPP's generated C layer. */
internal object JavaCppSupport {
  private val threadDiagnostic = ThreadLocal<String>()

  fun setThreadDiagnostic(diagnostic: String) {
    threadDiagnostic.set(diagnostic)
  }

  fun takeThreadDiagnostic(): String? {
    val diagnostic = threadDiagnostic.get()
    if (diagnostic != null) threadDiagnostic.remove()
    return diagnostic
  }

  fun cString(pointer: BytePointer?): String =
    if (pointer == null || pointer.isNull) "" else pointer.getString(StandardCharsets.UTF_8)

  fun utf8(value: String?): BytePointer? = value?.let { BytePointer(it, StandardCharsets.UTF_8) }

  fun requireValidCString(value: String) {
    if ('\u0000' in value) {
      throw Status.invalidArgument("C string inputs cannot contain embedded NUL characters")
    }
  }

  fun cString(value: String): BytePointer {
    requireValidCString(value)
    return BytePointer(value, StandardCharsets.UTF_8)
  }

  fun utf8String(pointer: Pointer?, byteLength: Long): String =
    String(byteArray(pointer, byteLength), StandardCharsets.UTF_8)

  fun byteArray(pointer: Pointer?, byteLength: Long): ByteArray {
    if (pointer == null || pointer.isNull || byteLength == 0L) {
      return ByteArray(0)
    }
    val bytes = ByteArray(Math.toIntExact(byteLength))
    BytePointer(pointer).get(bytes, 0, bytes.size)
    return bytes
  }
}
