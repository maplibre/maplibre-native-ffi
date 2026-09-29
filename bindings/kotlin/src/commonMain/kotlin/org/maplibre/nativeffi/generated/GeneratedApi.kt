// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*

public expect object GeneratedApi {
  public fun androidInit(
    jniEnv: org.maplibre.nativeffi.render.NativePointer,
    jniClass: org.maplibre.nativeffi.render.NativePointer,
    context: org.maplibre.nativeffi.render.NativePointer,
  ): Unit

  public fun animationOptionsDefault(): AnimationOptions

  public fun boundOptionsDefault(): BoundOptions

  public fun cVersion(): UInt

  public fun cameraDeltaDefault(): CameraDelta

  public fun cameraFitOptionsDefault(): CameraFitOptions

  public fun cameraOptionsDefault(): CameraOptions

  public fun cameraUpdateDefault(): CameraUpdate

  public fun customGeometrySourceOptionsDefault(): CustomGeometrySourceOptions

  public fun customMvtVectorSourceOptionsDefault(): CustomMvtVectorSourceOptions

  public fun frameDemandDefault(): FrameDemand

  public fun freeCameraOptionsDefault(): FreeCameraOptions

  public fun geojsonSourceDataCreate(
    data: ByteArray,
    options: GeojsonSourceOptions? = null,
  ): org.maplibre.nativeffi.generated.GeojsonSourceDataHandle

  public fun geojsonSourceOptionsDefault(): GeojsonSourceOptions

  public fun gpuSyncDefault(): GpuSync

  public fun httpHeaderTransformResponseSet(
    response: HttpHeaderTransformResponse,
    name: String,
    valueValue: String,
  ): Unit

  public fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng

  public fun logClearCallback(): Unit

  public fun logSetAsyncSeverityMask(mask: LogSeverityMask): Unit

  public fun logSetCallback(callback: LogCallback): Unit

  public fun mapOptionsDefault(): MapOptions

  public fun mapTileOptionsDefault(): MapTileOptions

  public fun mapViewportOptionsDefault(): MapViewportOptions

  public fun metalBorrowedTextureDescriptorDefault(): MetalBorrowedTextureDescriptor

  public fun metalOwnedTextureDescriptorDefault(): MetalOwnedTextureDescriptor

  public fun metalSurfaceDescriptorDefault(): MetalSurfaceDescriptor

  public fun networkStatusGet(): NetworkStatus

  public fun networkStatusSet(status: NetworkStatus): Unit

  public fun openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor

  public fun openglOwnedTextureDescriptorDefault(): OpenglOwnedTextureDescriptor

  public fun openglSupportedContextProviderMask(): OpenglContextProviderFlag

  public fun openglSurfaceDescriptorDefault(): OpenglSurfaceDescriptor

  public fun pluginGetRegisterFunctionV1(): org.maplibre.nativeffi.render.NativePointer

  public fun premultipliedRgba8ImageDefault(): PremultipliedRgba8Image

  public fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters

  public fun projectionModeDefault(): ProjectionMode

  public fun renderSessionAttachOptionsDefault(): RenderSessionAttachOptions

  public fun renderTargetExtentPhysicalSize(
    extent: RenderTargetExtent
  ): RenderTargetExtentPhysicalSizeResult

  public fun renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions

  public fun renderedQueryGeometryBox(box: ScreenBox): RenderedQueryGeometry

  public fun renderedQueryGeometryLineString(points: List<ScreenPoint>): RenderedQueryGeometry

  public fun renderedQueryGeometryPoint(point: ScreenPoint): RenderedQueryGeometry

  public fun resourceTransformResponseSetUrl(response: ResourceTransformResponse, url: String): Unit

  public fun runtimeCreate(options: RuntimeOptions): org.maplibre.nativeffi.generated.RuntimeHandle

  public fun runtimeOptionsDefault(): RuntimeOptions

  public fun sourceFeatureQueryOptionsDefault(): SourceFeatureQueryOptions

  public fun styleImageInfoDefault(): StyleImageInfo

  public fun styleImageOptionsDefault(): StyleImageOptions

  public fun styleTileSourceOptionsDefault(): StyleTileSourceOptions

  public fun styleTransitionOptionsDefault(): StyleTransitionOptions

  public fun supportedRenderBackendMask(): RenderBackendFlag

  public fun textureImageInfoDefault(): TextureImageInfo

  public fun vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor

  public fun vulkanOwnedTextureDescriptorDefault(): VulkanOwnedTextureDescriptor

  public fun vulkanSurfaceDescriptorDefault(): VulkanSurfaceDescriptor

  public fun webgpuBorrowedTextureDescriptorDefault(): WebgpuBorrowedTextureDescriptor

  public fun webgpuOwnedTextureDescriptorDefault(): WebgpuOwnedTextureDescriptor

  public fun webgpuSurfaceDescriptorDefault(): WebgpuSurfaceDescriptor
}
