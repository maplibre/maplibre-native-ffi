package org.maplibre.nativeffi.internal.javacpp

import java.nio.charset.StandardCharsets
import org.bytedeco.javacpp.BytePointer
import org.maplibre.nativeffi.internal.status.Status

internal class ByteArrayViewScope(bytes: ByteArray) : AutoCloseable {
  private val data =
    BytePointer(bytes.size.toLong()).also { pointer -> pointer.put(bytes, 0, bytes.size) }

  val view: MaplibreNativeC.mln_buffer_view =
    MaplibreNativeC.mln_buffer_view().also { value ->
      value.data(data)
      value.size(bytes.size.toLong())
    }

  override fun close() {
    view.close()
    data.close()
  }
}

internal class StringViewScope(value: String) : AutoCloseable {
  private val bytes: BytePointer
  val view: MaplibreNativeC.mln_buffer_view = MaplibreNativeC.mln_buffer_view()

  init {
    val utf8 = value.toByteArray(StandardCharsets.UTF_8)
    bytes = BytePointer(Math.max(utf8.size, 1).toLong())
    if (utf8.isNotEmpty()) bytes.put(utf8, 0, utf8.size)
    view.data(if (utf8.isEmpty()) null else bytes)
    view.size(utf8.size.toLong())
  }

  override fun close() {
    view.close()
    bytes.close()
  }
}

internal fun ownedBuffer(handle: Long): ByteArray =
  ownedBuffer(handle, MaplibreNativeC::mln_buffer_get, MaplibreNativeC::mln_buffer_destroy)

internal fun ownedBuffer(
  handle: Long,
  getter: (Long, MaplibreNativeC.mln_buffer_view) -> Int,
  destroyer: (Long) -> Unit,
): ByteArray {
  require(handle != 0L) { "native buffer handle is null" }
  try {
    MaplibreNativeC.mln_buffer_view().use { bytes ->
      Status.check(getter(handle, bytes))
      val size = Math.toIntExact(bytes.size())
      if (size == 0) return byteArrayOf()
      return ByteArray(size).also { output -> BytePointer(bytes.data()).position(0).get(output) }
    }
  } finally {
    destroyer(handle)
  }
}
