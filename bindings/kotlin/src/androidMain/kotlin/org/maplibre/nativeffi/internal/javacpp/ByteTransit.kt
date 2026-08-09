package org.maplibre.nativeffi.internal.javacpp

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

internal fun ownedBuffer(handle: Long): ByteArray {
  require(handle != 0L) { "native buffer handle is null" }
  try {
    MaplibreNativeC.mln_buffer_view().use { bytes ->
      Status.check(MaplibreNativeC.mln_buffer_get(handle, bytes))
      val size = Math.toIntExact(bytes.size())
      if (size == 0) return byteArrayOf()
      return ByteArray(size).also { output -> BytePointer(bytes.data()).position(0).get(output) }
    }
  } finally {
    MaplibreNativeC.mln_buffer_destroy(handle)
  }
}
