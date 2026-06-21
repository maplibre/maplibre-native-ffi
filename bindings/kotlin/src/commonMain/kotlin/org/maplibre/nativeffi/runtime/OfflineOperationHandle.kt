package org.maplibre.nativeffi.runtime

/** Owner-thread offline database operation that must be taken or discarded. */
public expect class OfflineOperationHandle<T> : AutoCloseable {
  /** Native `uint64_t` operation id preserved as a [Long] bit pattern. */
  public val id: Long

  public val kind: OfflineOperationKind

  public val resultKind: OfflineOperationResultKind

  public val isClosed: Boolean

  override fun close()
}
