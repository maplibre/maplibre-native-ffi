package org.maplibre.nativeffi.camera

/**
 * Mutable animation descriptor used for animated camera commands.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class AnimationOptions {
  public var durationMs: Double? = null

  public var velocity: Double? = null

  public var minZoom: Double? = null

  public var easing: UnitBezier? = null

  /**
   * Caller-chosen identity for the transition these options start.
   *
   * When set, the transition emits one
   * [org.maplibre.nativeffi.runtime.RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED] event carrying
   * this value in a [org.maplibre.nativeffi.runtime.RuntimeEventPayload.CameraTransitionFinished]
   * payload. MapLibre Native passes the value through without interpreting it, so callers pick
   * their own scheme, such as a monotonically increasing counter. Native `uint64_t` carried as a
   * [Long] bit pattern: an id above [Long.MAX_VALUE] passes through as `id.toLong()` and comes back
   * in the same form.
   *
   * Each transition emits that event exactly once, whichever way it ends: running to completion,
   * being superseded by a later camera command, being cancelled by
   * [org.maplibre.nativeffi.map.MapHandle.cancelTransitions], completing instantly as a
   * zero-duration jump, or exiting early because the requested camera contained a non-finite value.
   * MapLibre Native reports the moment a transition releases the camera and leaves the outcome
   * unreported, so the event establishes transition identity rather than a completion reason. A
   * host that needs to tell completion from cancellation compares the resulting camera against the
   * requested one, or tracks which transition id is current.
   *
   * The event is queued on the runtime that owns the map and is drained by
   * [org.maplibre.nativeffi.runtime.RuntimeHandle.pollEvent]. For a transition that runs to
   * completion, it is queued immediately before that transition's
   * [org.maplibre.nativeffi.runtime.RuntimeEventType.MAP_CAMERA_DID_CHANGE] event.
   *
   * Leaving this field null leaves the transition silent.
   */
  public var transitionId: Long? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: AnimationOptions.() -> Unit = {}): AnimationOptions =
    AnimationOptions()
      .also {
        it.durationMs = durationMs
        it.velocity = velocity
        it.minZoom = minZoom
        it.easing = easing
        it.transitionId = transitionId
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(durationMs, velocity, minZoom, easing, transitionId)

  override fun equals(other: Any?): Boolean = other is AnimationOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
