package org.maplibre.nativeffi.resource

/**
 * One route that a queued resource provider claims.
 *
 * A route compares [url] against the request's resolved URL, or against its requested URL when
 * [useRequestedUrl] is set. With [matchGlob] the comparison reads [url] as the glob pattern
 * language the C API reference defines. A null [kind] matches every resource kind.
 */
public class ResourceProviderRoute(
  public val url: String,
  public val kind: ResourceKind? = null,
  public val matchGlob: Boolean = false,
  public val useRequestedUrl: Boolean = false,
)

/**
 * Receives the requests that a queued resource provider's routes claim.
 *
 * The binding invokes this from `pump`, on the thread the runtime runs on, rather than on the
 * MapLibre thread that produced the request. Complete or close [handle] to answer, from this call
 * or from a later one.
 */
public fun interface QueuedResourceProviderCallback {
  public fun handle(request: ResourceRequest, handle: ResourceRequestHandle)
}

/**
 * One rule in a resource URL rewrite table.
 *
 * A rule compares [url] against the request URL, reading it as a glob pattern when [matchGlob] is
 * set, and the first matching rule replaces the URL with [replacementUrl]. A null [replacementUrl]
 * leaves the URL unchanged, and a null [kind] matches every resource kind.
 */
public class ResourceUrlRewriteRule(
  public val url: String,
  public val replacementUrl: String?,
  public val kind: ResourceKind? = null,
  public val matchGlob: Boolean = false,
)
