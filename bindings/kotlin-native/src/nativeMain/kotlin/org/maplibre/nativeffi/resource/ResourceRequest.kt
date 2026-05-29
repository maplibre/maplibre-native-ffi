package org.maplibre.nativeffi.resource

/** Copied network resource request passed to a runtime resource provider. */
public class ResourceRequest(
  public val url: String,
  public val kind: ResourceKind,
  public val rawKind: Int,
  public val loadingMethod: ResourceLoadingMethod,
  public val rawLoadingMethod: Int,
  public val priority: ResourcePriority,
  public val rawPriority: Int,
  public val usage: ResourceUsage,
  public val rawUsage: Int,
  public val storagePolicy: ResourceStoragePolicy,
  public val rawStoragePolicy: Int,
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
