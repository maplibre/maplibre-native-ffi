package org.maplibre.nativeffi.internal.javacpp

import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.map.JavaCppMapStructs
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.ProjectionModeOptions
import org.maplibre.nativeffi.map.TileOptions
import org.maplibre.nativeffi.map.ViewportOptions
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.render.JavaCppRenderStructs
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.resource.JavaCppResourceStructs
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.runtime.JavaCppRuntimeStructs
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions

/** Testable entry points for the handwritten JavaCPP descriptor adapter. */
internal object JavaCppStructs {
  fun stringViewRoundTrip(value: String): String = JavaCppMapStructs.stringViewRoundTrip(value)

  fun cameraOptionsRoundTrip(value: CameraOptions): Pair<Int, CameraOptions> =
    JavaCppMapStructs.cameraOptionsRoundTrip(value)

  fun animationOptionsSnapshot(
    value: AnimationOptions
  ): JavaCppMapStructs.AnimationOptionsSnapshot = JavaCppMapStructs.animationOptionsSnapshot(value)

  fun viewportOptionsRoundTrip(value: ViewportOptions): ViewportOptions =
    JavaCppMapStructs.viewportOptionsRoundTrip(value)

  fun tileOptionsRoundTrip(value: TileOptions): TileOptions =
    JavaCppMapStructs.tileOptionsRoundTrip(value)

  fun projectionModeOptionsRoundTrip(value: ProjectionModeOptions): ProjectionModeOptions =
    JavaCppMapStructs.projectionModeOptionsRoundTrip(value)

  fun renderedQueryGeometryType(value: RenderedQueryGeometry): Int =
    JavaCppRenderStructs.renderedQueryGeometryType(value)

  fun offlineRegionDefinitionRoundTrip(value: OfflineRegionDefinition): OfflineRegionDefinition =
    JavaCppRuntimeStructs.offlineRegionDefinitionRoundTrip(value)

  fun offlineRegionInfoSnapshot(
    id: Long,
    definition: OfflineRegionDefinition,
    metadata: ByteArray,
  ): OfflineRegionInfo = JavaCppRuntimeStructs.offlineRegionInfoSnapshot(id, definition, metadata)

  fun unknownRuntimePayload(type: Int, bytes: ByteArray): RuntimeEventPayload =
    JavaCppRuntimeStructs.unknownRuntimePayload(type, bytes)

  fun styleImageOptionsSnapshot(
    value: StyleImageOptions
  ): JavaCppMapStructs.StyleImageOptionsSnapshot =
    JavaCppMapStructs.styleImageOptionsSnapshot(value)

  fun styleImageInfoSnapshot(byteLength: Long): StyleImageInfo =
    JavaCppMapStructs.styleImageInfoSnapshot(byteLength)

  fun sourceInfoSnapshot(type: Int, volatileSource: Boolean): SourceInfo =
    JavaCppMapStructs.sourceInfoSnapshot(type, volatileSource)

  fun textureImageInfoSnapshot(
    width: Int,
    height: Int,
    stride: Int,
    byteLength: Long,
  ): TextureImageInfo =
    JavaCppRenderStructs.textureImageInfoSnapshot(width, height, stride, byteLength)

  fun mapOptionsSnapshot(value: MapOptions): JavaCppMapStructs.MapOptionsSnapshot =
    JavaCppMapStructs.mapOptionsSnapshot(value)

  fun metalSnapshot(
    value: MetalBorrowedTextureDescriptor
  ): JavaCppRenderStructs.RenderDescriptorSnapshot = JavaCppRenderStructs.metalSnapshot(value)

  fun vulkanSnapshot(
    value: VulkanBorrowedTextureDescriptor
  ): JavaCppRenderStructs.RenderDescriptorSnapshot = JavaCppRenderStructs.vulkanSnapshot(value)

  fun openGlSnapshot(
    value: OpenGLBorrowedTextureDescriptor
  ): JavaCppRenderStructs.RenderDescriptorSnapshot = JavaCppRenderStructs.openGlSnapshot(value)

  fun resourceResponseSnapshot(
    value: ResourceResponse
  ): JavaCppResourceStructs.ResourceResponseSnapshot =
    JavaCppResourceStructs.resourceResponseSnapshot(value)
}
