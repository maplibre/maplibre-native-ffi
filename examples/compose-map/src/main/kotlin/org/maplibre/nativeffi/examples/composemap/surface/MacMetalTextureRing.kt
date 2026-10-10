package org.maplibre.nativeffi.examples.composemap.surface

/**
 * The ring of [RING_DEPTH] Skiko Metal textures that a macOS bridge lends the producer, directly or
 * through an import into the producer's API. The renderer holds the frame the consumer draws until
 * a newer one arrives, so the session renders into another texture meanwhile.
 *
 * A bridge allocates a new ring on every resize, and the map needs a frame or two to fill it. The
 * ring a frame last landed in stays alive as the retired ring until a frame lands in its
 * replacement, and the consumer draws it in between.
 *
 * The renderer thread allocates and releases textures, and the Compose thread draws them.
 */
internal class MacMetalTextureRing {
  @Volatile
  var textures: List<NativeHandle> = emptyList()
    private set

  @Volatile
  var device: NativeHandle = NativeHandle(0)
    private set

  @Volatile
  var pixelFormat: Long = 0L
    private set

  /** Counts up on every allocation, so a frame names the ring it landed in. */
  @Volatile
  var generation: Long = 0L
    private set

  @Volatile private var renderedGeneration = 0L

  @Volatile private var retired: List<NativeHandle> = emptyList()
  @Volatile private var retiredPixelFormat = 0L
  @Volatile private var retiredGeneration = 0L

  val isEmpty: Boolean
    get() = textures.isEmpty()

  /**
   * Allocates a ring at [extent] on [skikoDevice] and counts [generation] up. Skiko keeps a texture
   * whose size already matches, and only the device that allocated a texture can take it back, so a
   * device change allocates every texture anew.
   */
  fun allocate(extent: SurfaceExtent, skikoDevice: SkikoMetalDevice) {
    val outgoing = textures
    val sameDevice = device.address == skikoDevice.ptr
    val replacement =
      List(RING_DEPTH) { slot ->
        val reusable = if (sameDevice) outgoing.getOrNull(slot)?.address ?: 0L else 0L
        NativeHandle(
          MacMetalBridgeNative.createMetalTexture(
            metalDevice = skikoDevice.ptr,
            oldTexture = reusable,
            width = extent.physicalWidth,
            height = extent.physicalHeight,
          )
        )
      }
    if (replacement != outgoing) {
      retire(outgoing, deviceChanged = !sameDevice)
    }
    textures = replacement
    device = NativeHandle(skikoDevice.ptr)
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(replacement.first().address)
    generation += 1
  }

  /** Records that a frame landed in the ring of [generation]. */
  fun markRendered(generation: Long) {
    renderedGeneration = generation
  }

  /**
   * Releases the retired ring once a frame landed in its replacement, a frame later, so the
   * consumer's last recorded frame from the retired ring has been flushed.
   */
  fun releaseRetiredOnceReplaced() {
    if (retired.isEmpty() || renderedGeneration != generation) {
      return
    }
    releaseRetired()
  }

  /**
   * The Metal texture to draw for [target], whose generation and slot name a ring texture, or null
   * when this ring no longer holds it.
   */
  fun drawable(target: NativeSurfaceTarget, origin: TextureOrigin): MetalTextureTarget? {
    val held: List<NativeHandle>
    val format: Long
    when (target.generation) {
      generation -> {
        held = textures
        format = pixelFormat
      }
      retiredGeneration -> {
        held = retired
        format = retiredPixelFormat
      }
      else -> return null
    }
    val texture = held.getOrNull(target.slotIndex) ?: return null
    if (texture.address == 0L) {
      return null
    }
    return MetalTextureTarget(
      texture = texture,
      device = device,
      pixelFormat = format,
      origin = origin,
      extent = target.extent,
      generation = target.generation,
      ring = held,
      slotIndex = target.slotIndex,
    )
  }

  /** Releases every texture, and counts [generation] up so no earlier frame draws. */
  fun dispose() {
    textures.forEach(::releaseMetalTexture)
    textures = emptyList()
    releaseRetired()
    device = NativeHandle(0)
    pixelFormat = 0
    generation += 1
  }

  // Holds the outgoing ring for the consumer to draw while the replacement is still empty. A
  // texture from a device Skiko has replaced cannot be drawn on the new one.
  private fun retire(outgoing: List<NativeHandle>, deviceChanged: Boolean) {
    if (outgoing.isEmpty()) {
      return
    }
    if (deviceChanged) {
      // Both belong to the device Skiko replaced.
      outgoing.forEach(::releaseMetalTexture)
      releaseRetired()
      return
    }
    if (renderedGeneration != generation) {
      // This one never held a frame, so whatever is already retired stays.
      outgoing.forEach(::releaseMetalTexture)
      return
    }
    releaseRetired()
    retired = outgoing
    retiredPixelFormat = pixelFormat
    retiredGeneration = generation
  }

  private fun releaseRetired() {
    retired.forEach(::releaseMetalTexture)
    retired = emptyList()
    retiredPixelFormat = 0
    retiredGeneration = 0
  }

  private fun releaseMetalTexture(texture: NativeHandle) {
    if (texture.address == 0L) {
      return
    }
    SkikoHost.forgetMetalTexture(texture)
    MacMetalBridgeNative.disposeMetalTexture(texture.address)
  }

  companion object {
    /** Two textures: one the consumer draws, and one the session renders into meanwhile. */
    const val RING_DEPTH: Int = 2
  }
}
