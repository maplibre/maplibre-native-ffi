package org.maplibre.nativeffi.resource

/** Mutable descriptor used to complete a resource provider request. */
public class ResourceResponse private constructor(public val status: ResourceResponseStatus) {
  public var errorReason: ResourceErrorReason = ResourceErrorReason.NONE
    private set

  private var responseBytes: ByteArray = ByteArray(0)

  public val bytes: ByteArray
    get() = responseBytes.copyOf()

  public var errorMessage: String? = null
    private set

  public var mustRevalidate: Boolean = false
    private set

  public var modifiedUnixMs: Long? = null
    private set

  public var expiresUnixMs: Long? = null
    private set

  public var etag: String? = null
    private set

  public var retryAfterUnixMs: Long? = null
    private set

  public fun errorReason(errorReason: ResourceErrorReason): ResourceResponse = apply {
    this.errorReason = errorReason
  }

  public fun bytes(bytes: ByteArray?): ResourceResponse = apply {
    responseBytes = bytes?.copyOf() ?: ByteArray(0)
  }

  public fun errorMessage(errorMessage: String): ResourceResponse = apply {
    this.errorMessage = errorMessage
  }

  public fun mustRevalidate(mustRevalidate: Boolean): ResourceResponse = apply {
    this.mustRevalidate = mustRevalidate
  }

  public fun modifiedUnixMs(modifiedUnixMs: Long): ResourceResponse = apply {
    this.modifiedUnixMs = modifiedUnixMs
  }

  public fun expiresUnixMs(expiresUnixMs: Long): ResourceResponse = apply {
    this.expiresUnixMs = expiresUnixMs
  }

  public fun etag(etag: String): ResourceResponse = apply { this.etag = etag }

  public fun retryAfterUnixMs(retryAfterUnixMs: Long): ResourceResponse = apply {
    this.retryAfterUnixMs = retryAfterUnixMs
  }

  public companion object {
    public fun ok(bytes: ByteArray): ResourceResponse =
      ResourceResponse(ResourceResponseStatus.OK).bytes(bytes)

    public fun noContent(): ResourceResponse = ResourceResponse(ResourceResponseStatus.NO_CONTENT)

    public fun notModified(): ResourceResponse =
      ResourceResponse(ResourceResponseStatus.NOT_MODIFIED)

    public fun error(reason: ResourceErrorReason, message: String): ResourceResponse =
      ResourceResponse(ResourceResponseStatus.ERROR).errorReason(reason).errorMessage(message)
  }
}
