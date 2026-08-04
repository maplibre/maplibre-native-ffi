package org.maplibre.nativeffi.runtime

/**
 * Scaffold for the browser offline database operation handle.
 *
 * Every member throws. The actual exists so the `wasmJs` source set compiles while the browser
 * binding is filled in one file at a time; nothing here is finished work.
 */
public actual class OfflineOperationHandle<T> private constructor() : AutoCloseable {
  public actual val id: Long
    get() = throw NotImplementedError("wasmJs OfflineOperationHandle.id is not implemented yet")

  public actual val kind: OfflineOperationKind
    get() = throw NotImplementedError("wasmJs OfflineOperationHandle.kind is not implemented yet")

  public actual val resultKind: OfflineOperationResultKind
    get() =
      throw NotImplementedError("wasmJs OfflineOperationHandle.resultKind is not implemented yet")

  public actual val isClosed: Boolean
    get() =
      throw NotImplementedError("wasmJs OfflineOperationHandle.isClosed is not implemented yet")

  public actual override fun close() {
    throw NotImplementedError("wasmJs OfflineOperationHandle.close is not implemented yet")
  }
}
