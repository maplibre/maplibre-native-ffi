package org.maplibre.nativeffi.resource

/** Owns a provider request until explicit close or collection. */
public expect class ResourceRequestHandle :
  org.maplibre.nativeffi.generated.GeneratedResourceRequestHandleOperations, AutoCloseable {
  override fun close()
}
