package org.maplibre.nativeffi.render

/** Explicit byte buffer for reusable native readback and upload storage. */
public expect class NativeBuffer : AutoCloseable {
  public fun byteLength(): Long

  public fun toByteArray(): ByteArray

  override fun close()

  public companion object {
    public fun allocate(byteLength: Long): NativeBuffer
  }
}
