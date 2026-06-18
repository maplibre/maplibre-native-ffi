package org.maplibre.nativeffi.resource

/** Mutable descriptor used to complete a resource provider request. */
public class ResourceResponse(public val status: ResourceResponseStatus) {
  public var errorReason: ResourceErrorReason = ResourceErrorReason.NONE

  private var responseBytes: ByteArray = ByteArray(0)

  public var bytes: ByteArray
    get() = responseBytes.copyOf()
    set(value) {
      responseBytes = value.copyOf()
    }

  public var errorMessage: String? = null

  public var mustRevalidate: Boolean = false

  public var modifiedUnixMs: Long? = null

  public var expiresUnixMs: Long? = null

  public var etag: String? = null

  public var retryAfterUnixMs: Long? = null
}
