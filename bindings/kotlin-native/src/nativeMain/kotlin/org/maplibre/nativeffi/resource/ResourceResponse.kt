package org.maplibre.nativeffi.resource

/** Mutable descriptor used to complete a resource provider request. */
public class ResourceResponse private constructor(public val status: ResourceResponseStatus) {
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

  public companion object {
    public fun ok(bytes: ByteArray): ResourceResponse =
      ResourceResponse(ResourceResponseStatus.OK).apply { this.bytes = bytes }

    public fun noContent(): ResourceResponse = ResourceResponse(ResourceResponseStatus.NO_CONTENT)

    public fun notModified(): ResourceResponse =
      ResourceResponse(ResourceResponseStatus.NOT_MODIFIED)

    public fun error(reason: ResourceErrorReason, message: String): ResourceResponse =
      ResourceResponse(ResourceResponseStatus.ERROR).apply {
        errorReason = reason
        errorMessage = message
      }
  }
}
