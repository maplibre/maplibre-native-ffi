package org.maplibre.nativeffi.internal.javacpp

import org.bytedeco.javacpp.BytePointer

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
