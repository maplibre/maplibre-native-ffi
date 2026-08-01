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
   * owner and throw `UnsupportedFeatureException`: allocate a texture at the new size and hand it
   * over with the backend's set-target method, such as [setMetalBorrowedTextureTarget], which keeps
   * this session.
   *
   * The session keeps its renderer across a resize, so renderer-held state such as feature state
   * carries over. A scale factor change is the exception: a renderer compiles its shaders for one
   * pixel ratio, so that resize starts a new one with renderer-held state empty. Map state such as
   * camera, style, and sources lives on the map and survives either way.
   */
  public fun resize(width: Int, height: Int, scaleFactor: Double)

  /**
   * Presents this attached surface session through a new surface.
   *
   * A host surface can be destroyed and recreated while the map goes on living, which is what
   * Android rotation, a Flutter `SurfaceProducer` lifecycle change, and a window resize that
   * reallocates all look like from here. Replacing the surface in place keeps this session's
   * renderer, and with it the tile pyramid, glyph and image atlases, symbol placement, and feature
   * state.
   *
   * [descriptor] names the graphics context this session attached with, and its extent applies as
   * [resize] applies one, including a scale factor change starting a new renderer. A descriptor
   * whose `context.device` is neither null nor this session's device throws
   * `InvalidArgumentException` and leaves this session rendering into the surface it has. The
   * session assigns the layer its own device and pixel format, so the layer itself carries nothing
   * that has to match.
   */
  public fun setMetalSurfaceTarget(descriptor: MetalSurfaceDescriptor)

  /**
   * Presents this attached surface session through a new surface.
   *
   * See [setMetalSurfaceTarget] for what replacing a surface preserves. The outgoing `VkSurfaceKHR`
   * must still be valid: this session holds a swapchain built from it, and Vulkan destroys every
   * swapchain before its surface. A host that has to release its surface first closes this session
   * and attaches again instead.
   *
   * The replacement reports the color format and surface-transform support this session compiled a
   * render pass and shaders for; one that does not throws `UnsupportedFeatureException`.
   */
  public fun setVulkanSurfaceTarget(descriptor: VulkanSurfaceDescriptor)

  /**
   * Presents this attached surface session through a new surface.
   *
   * See [setMetalSurfaceTarget] for what replacing a surface preserves. The new surface is made
   * current on the next render, so a host may hand over a replacement for one it has already
   * destroyed. Nothing is made current here, so a surface this call accepts can still prove
   * unusable; the next [renderUpdate] reports that rather than this call.
   */
  public fun setOpenGLSurfaceTarget(descriptor: OpenGLSurfaceDescriptor)

  /**
   * Renders this attached texture session into a new caller-owned texture.
   *
   * A caller-owned texture is sized by its owner, so a host that follows a resize reallocates
   * rather than resizing and [resize] throws `UnsupportedFeatureException`. Handing the replacement
   * over here keeps this session's renderer instead, and with it the tile pyramid, glyph and image
   * atlases, symbol placement, and feature state. A scale factor change is the exception, starting
   * a new renderer for the new pixel ratio just as [resize] does.
   *
   * The replacement belongs to the device this session attached with and carries the pixel format
   * it attached with; another device throws `InvalidArgumentException` and another pixel format
   * throws `UnsupportedFeatureException`, both leaving this session rendering into the texture it
   * has. The caller owns the replacement and keeps it valid until the next replacement, [detach],
   * or [close]. This session never retained the outgoing texture and never releases it, so the
   * caller is free to release that one once this call returns.
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
   * Renders the latest available map render update.
   *
   * The map retains its latest update, so repeated calls re-render it and return true again; use
   * this to redraw on demand after resize or surface expose, and gate frame loops on
   * render-update-available events instead of the return value. Returns false when no frame was
   * rendered, because the map has not published an update yet or the renderer skipped the frame;
   * both are normal during startup, so keep pumping the runtime until an update is reported.
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
   * `offset` arguments as [JsonValue.UInt]. Other numeric types are treated as absent: a
   * `cluster_id` that is not [JsonValue.UInt] returns [FeatureExtensionResult.Value] holding
   * [JsonValue.Null] instead of a feature collection, and a `limit` or `offset` that is not
   * [JsonValue.UInt] leaves `leaves` at the native defaults of ten leaves at offset zero.
   * [QueriedFeature] properties keep their JSON value type, so a queried cluster feature can be
   * passed back unmodified.
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
