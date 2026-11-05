package org.maplibre.nativeffi

import kotlin.jvm.JvmName

/** Error for a status that the native MapLibre C ABI reported, or that the binding raised. */
public open class MaplibreException
internal constructor(
  public val status: Status,
  public val diagnostic: String,
  /**
   * The map snapshot generation that a failed or cancelled map command published, and zero for any
   * other failure.
   */
  @get:JvmName("getGeneration") public val generation: ULong = 0uL,
) : RuntimeException("$status: ${diagnostic.ifBlank { "No native diagnostic available." }}")
