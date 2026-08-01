package org.maplibre.nativeffi.resource

/** Copied request passed to a runtime HTTP header transform. */
public data class HttpHeaderTransformRequest(public val kind: ResourceKind, public val url: String)

/** Header copied into a built-in HTTP request. */
public data class HttpHeader(public val name: String, public val value: String)

public fun interface HttpHeaderTransformCallback {
  public fun transform(request: HttpHeaderTransformRequest): List<HttpHeader>
}
