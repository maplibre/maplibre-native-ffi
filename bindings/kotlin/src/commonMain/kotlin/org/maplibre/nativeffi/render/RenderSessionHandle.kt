package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Owned render session handle. Platform actuals own the native render session carrier. */
public expect class RenderSessionHandle : AutoCloseable {
  public val isClosed: Boolean

  public fun map(): MapHandle

  /**
   * Resizes this attached render session.
   *
   * Surface and owned-texture sessions resize in place. Borrowed texture targets are sized by their
   * owner and throw `UnsupportedFeatureException`; hand over a new texture with the backend's
   * set-target method, such as [setMetalBorrowedTextureTarget].
   *
   * The session keeps its renderer across a resize, so renderer-held state such as feature state
   * carries over. A scale factor change starts a new renderer with that state empty, since shaders
   * are compiled for one pixel ratio. Map state such as camera, style, and sources survives either
   * way.
   */
  public fun resize(width: Int, height: Int, scaleFactor: Double)

  /**
   * Presents this attached surface session through a new surface, keeping this session's renderer
   * and the state it holds.
   *
   * [descriptor] names the graphics context this session attached with, and its extent applies as
   * [resize] applies one, including a scale factor change starting a new renderer. A descriptor
   * whose `context.device` is neither null nor this session's device throws
   * `InvalidArgumentException` and leaves this session rendering into the surface it has.
   */
  public fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor)

  /**
   * Presents this attached surface session through a new surface.
   *
   * See [setMetalSurfaceTarget] for what replacing a surface preserves. The outgoing `VkSurfaceKHR`
   * must still be valid, since this session holds a swapchain built from it. A host that must
   * release its surface first closes this session and attaches again instead.
   *
   * The replacement must report the color format and surface-transform support this session
   * compiled its render pass and shaders for; one that does not throws
   * `UnsupportedFeatureException`.
   */
  public fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor)

  /**
   * Presents this attached surface session through a new surface.
   *
   * See [setMetalSurfaceTarget] for what replacing a surface preserves. The new surface is made
   * current on the next render, so a host may hand over a replacement for one it has already
   * destroyed, and an unusable surface is reported by the next [renderUpdate] rather than here.
   */
  public fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor)

  /**
   * Renders this attached texture session into a new caller-owned texture, keeping this session's
   * renderer and the state it holds. A scale factor change starts a new renderer, as in [resize].
   *
   * The replacement must belong to the device this session attached with and carry the pixel format
   * it attached with; another device throws `InvalidArgumentException` and another pixel format
   * throws `UnsupportedFeatureException`, both leaving this session rendering into the texture it
   * has. The caller owns the replacement and keeps it valid until the next replacement, [detach],
   * or [close]. This session never retains or reads the outgoing texture.
   */
  public fun setMetalBorrowedTextureTarget(descriptor: MetalBorrowedTextureDescriptor)

  /**
   * Renders this attached texture session into a new caller-owned image.
   *
   * See [setMetalBorrowedTextureTarget] for what replacing a target preserves. The replacement
   * carries the format and both layouts this session attached with, since its render pass was built
   * around them.
   */
  public fun setVulkanBorrowedTextureTarget(descriptor: VulkanBorrowedTextureDescriptor)

  /**
   * Renders this attached texture session into a new caller-owned texture.
   *
   * See [setMetalBorrowedTextureTarget] for what replacing a target preserves. The replacement
   * belongs to the context this session attached with, or one in its share group, and that context
   * must be current on this thread.
   */
  public fun setOpenGLBorrowedTextureTarget(descriptor: OpenGLBorrowedTextureDescriptor)

  /**
   * Renders the latest available map render update, returning false when no frame was rendered.
   *
   * The map retains its latest update, so repeated calls re-render it. False is a normal transient:
   * call again on the next frame rather than wait for another render-update event.
   */
  public fun renderUpdate(): Boolean

  public fun detach()

  public fun reduceMemoryUse()

  public fun clearData()

  public fun dumpDebugLogs()

  public fun setFeatureState(selector: FeatureStateSelector, value: JsonValue)

  public fun getFeatureState(selector: FeatureStateSelector): JsonValue

  public fun removeFeatureState(selector: FeatureStateSelector)

  public fun queryRenderedFeatures(
    geometry: RenderedQueryGeometry,
    options: RenderedFeatureQueryOptions?,
  ): List<QueriedFeature>

  public fun querySourceFeatures(
    sourceId: String,
    options: SourceFeatureQueryOptions?,
  ): List<QueriedFeature>

  /**
   * Queries a feature extension from the latest render session state.
   *
   * The `supercluster` extension reads the `cluster_id` feature property and the `limit` and
   * `offset` arguments as [JsonValue.UInt]; other numeric types are treated as absent.
   */
  public fun queryFeatureExtension(
    sourceId: String,
    feature: Feature,
    extension: String,
    extensionField: String,
    arguments: JsonValue?,
  ): FeatureExtensionResult

  public fun textureImageInfo(): TextureImageInfo

  public fun readPremultipliedRgba8(buffer: NativeBuffer): TextureImageInfo

  public fun acquireMetalOwnedTextureFrame(): MetalOwnedTextureFrameHandle

  public fun acquireVulkanOwnedTextureFrame(): VulkanOwnedTextureFrameHandle

  public fun acquireOpenGLOwnedTextureFrame(): OpenGLOwnedTextureFrameHandle

  override fun close()
}
