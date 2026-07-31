package org.maplibre.nativeffi.resource

/** Copied network resource request passed to a runtime resource provider. */
public class ResourceRequest(
  /** URL entering the network layer, preserving configured scheme aliases. */
  public val requestedUrl: String,
  /** URL to fetch, after tile server normalization. */
  public val resolvedUrl: String,
  public val kind: ResourceKind,
  public val loadingMethod: ResourceLoadingMethod,
  public val priority: ResourcePriority,
  public val usage: ResourceUsage,
  public val storagePolicy: ResourceStoragePolicy,
  public val range: ByteRange?,
  public val priorModifiedUnixMs: Long?,
  public val priorExpiresUnixMs: Long?,
  public val priorEtag: String?,
  priorData: ByteArray,
) {
  private val priorDataBytes: ByteArray = priorData.copyOf()

  public val priorData: ByteArray
    get() = priorDataBytes.copyOf()

  /** HTTP byte range. Values preserve native `uint64_t` bit patterns in [Long]. */
  public data class ByteRange(public val start: Long, public val end: Long)
}
