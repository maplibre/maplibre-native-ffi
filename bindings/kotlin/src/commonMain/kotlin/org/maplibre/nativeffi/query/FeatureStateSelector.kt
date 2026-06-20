package org.maplibre.nativeffi.query

import org.maplibre.nativeffi.internal.status.Status

/** Mutable selector for render-session feature-state operations. */
public class FeatureStateSelector(public val sourceId: String) {
  public var sourceLayerId: String? = null

  public var featureId: String? = null
    set(value) {
      field = value
      if (value == null) stateKey = null
    }

  public var stateKey: String? = null
    set(value) {
      Status.requireArgument(value == null || featureId != null) { "stateKey requires featureId" }
      field = value
    }
}
