package org.maplibre.nativeffi.resource

/**
 * Scaffold for the browser resource provider request handle.
 *
 * Every member throws. The actual exists so the `wasmJs` source set compiles while the browser
 * binding is filled in one file at a time; nothing here is finished work.
 */
public actual class ResourceRequestHandle private constructor() : AutoCloseable {
  public actual fun complete(response: ResourceResponse) {
    throw NotImplementedError("wasmJs ResourceRequestHandle.complete is not implemented yet")
  }

  public actual fun isCancelled(): Boolean =
    throw NotImplementedError("wasmJs ResourceRequestHandle.isCancelled is not implemented yet")

  public actual override fun close() {
    throw NotImplementedError("wasmJs ResourceRequestHandle.close is not implemented yet")
  }
}
