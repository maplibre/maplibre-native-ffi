// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public expect class ResourceRequestHandle :
  GeneratedResourceRequestHandleOperations, AutoCloseable {
  public val isClosed: Boolean

  override fun close()
}
