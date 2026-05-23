package org.maplibre.nativeffi.query

/** Mutable selector for render-session feature-state operations. */
public class FeatureStateSelector(public val sourceId: String) {
  public var sourceLayerId: String? = null
    private set

  public var featureId: String? = null
    private set

  public var stateKey: String? = null
    private set

  public fun hasSourceLayerId(): Boolean = sourceLayerId != null

  public fun sourceLayerId(sourceLayerId: String): FeatureStateSelector = apply {
    this.sourceLayerId = sourceLayerId
  }

  public fun clearSourceLayerId(): FeatureStateSelector = apply { sourceLayerId = null }

  public fun hasFeatureId(): Boolean = featureId != null

  public fun featureId(featureId: String): FeatureStateSelector = apply {
    this.featureId = featureId
  }

  public fun clearFeatureId(): FeatureStateSelector = apply {
    featureId = null
    stateKey = null
  }

  public fun hasStateKey(): Boolean = stateKey != null

  public fun stateKey(stateKey: String): FeatureStateSelector = apply {
    check(featureId != null) { "stateKey requires featureId" }
    this.stateKey = stateKey
  }

  public fun clearStateKey(): FeatureStateSelector = apply { stateKey = null }
}
