package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.javacpp.JavaCppStructs
import org.maplibre.nativeffi.map.ConstrainMode
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.map.NorthOrientation
import org.maplibre.nativeffi.map.ProjectionModeOptions
import org.maplibre.nativeffi.map.TileLodMode
import org.maplibre.nativeffi.map.TileOptions
import org.maplibre.nativeffi.map.ViewportMode
import org.maplibre.nativeffi.map.ViewportOptions
import org.maplibre.nativeffi.offline.OfflineRegionDefinition
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.render.EglContextDescriptor
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.TextureImageInfo
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanContextDescriptor
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.style.StyleImageOptions

class JavaCppStructsTest {
  @Test
  fun stringViewsPreserveEmbeddedNulBytes() {
    assertEquals("value\u0000suffix", JavaCppStructs.stringViewRoundTrip("value\u0000suffix"))
  }

  @Test
  fun optionDescriptorsPreservePresentZeroFields() {
    val camera =
      CameraOptions().apply {
        center = LatLng(0.0, 0.0)
        zoom = 0.0
        anchor = ScreenPoint(0.0, 0.0)
      }
    val (fields, copy) = JavaCppStructs.cameraOptionsRoundTrip(camera)
    assertNotEquals(0, fields)
    assertEquals(camera, copy)

    val animation =
      JavaCppStructs.animationOptionsSnapshot(
        AnimationOptions().apply {
          durationMs = 0.0
          velocity = 0.0
          transitionId = -1
        }
      )
    assertNotEquals(0, animation.fields)
    assertEquals(0.0, animation.durationMs)
    assertEquals(0.0, animation.velocity)
    assertEquals(-1L, animation.transitionId)

    val viewport =
      ViewportOptions().apply {
        northOrientation = NorthOrientation.LEFT
        constrainMode = ConstrainMode.SCREEN
        viewportMode = ViewportMode.FLIPPED_Y
      }
    assertEquals(viewport, JavaCppStructs.viewportOptionsRoundTrip(viewport))
    val tile = TileOptions().apply { lodMode = TileLodMode.DISTANCE }
    assertEquals(tile, JavaCppStructs.tileOptionsRoundTrip(tile))
    val projection =
      ProjectionModeOptions().apply {
        axonometric = false
        xSkew = 0.0
        ySkew = 0.0
      }
    assertEquals(projection, JavaCppStructs.projectionModeOptionsRoundTrip(projection))
  }

  @Test
  fun queryGeometryMaterializesEveryDiscriminator() {
    val point =
      JavaCppStructs.renderedQueryGeometryType(RenderedQueryGeometry.Point(ScreenPoint(1.0, 2.0)))
    val box =
      JavaCppStructs.renderedQueryGeometryType(
        RenderedQueryGeometry.Box(ScreenBox(ScreenPoint(1.0, 2.0), ScreenPoint(3.0, 4.0)))
      )
    val line =
      JavaCppStructs.renderedQueryGeometryType(
        RenderedQueryGeometry.LineString(listOf(ScreenPoint(1.0, 2.0), ScreenPoint(3.0, 4.0)))
      )
    assertEquals(3, setOf(point, box, line).size)
  }

  @Test
  fun featureStateSelectorEmbedsStringViewsByValue() {
    val minimal = JavaCppStructs.featureStateSelectorSnapshot(FeatureStateSelector("point"))
    assertEquals("point", minimal.sourceId)
    assertEquals(0, minimal.fields)
    assertEquals(null, minimal.sourceLayerId)
    assertEquals(null, minimal.featureId)
    assertEquals(null, minimal.stateKey)

    val full =
      JavaCppStructs.featureStateSelectorSnapshot(
        FeatureStateSelector("point").apply {
          sourceLayerId = "layer"
          featureId = "feature-1"
          stateKey = "hover"
        }
      )
    assertEquals("point", full.sourceId)
    assertEquals("layer", full.sourceLayerId)
    assertEquals("feature-1", full.featureId)
    assertEquals("hover", full.stateKey)
    assertNotEquals(0, full.fields)
  }

  @Test
  fun ownedBufferIsDestroyedAfterCopyFailure() {
    assertEquals(1, JavaCppStructs.ownedBufferCleanupAfterCopyFailure())
  }

  @Test
  fun offlineDefinitionsAndUnknownRuntimePayloadsCopyOwnedData() {
    val definition =
      OfflineRegionDefinition.TilePyramid(
        "asset://style.json",
        LatLngBounds(LatLng(1.0, 2.0), LatLng(3.0, 4.0)),
        1.0,
        5.0,
        2.0f,
        true,
      )
    assertEquals(definition, JavaCppStructs.offlineRegionDefinitionRoundTrip(definition))
    val info = JavaCppStructs.offlineRegionInfoSnapshot(7, definition, byteArrayOf(1, 2, 3))
    assertEquals(7, info.id)
    assertEquals(definition, info.definition)
    assertContentEquals(byteArrayOf(1, 2, 3), info.metadata)

    val payload =
      JavaCppStructs.unknownRuntimePayload(999, byteArrayOf(1, 2, 3)) as RuntimeEventPayload.Unknown
    assertEquals(999, payload.rawPayloadType)
    assertContentEquals(byteArrayOf(1, 2, 3), payload.payloadBytes)
    assertEquals(1, JavaCppStructs.offlineRegionListCleanupAfterCopyFailure())
  }

  @Test
  fun styleAndTextureImageReadersRejectOverflowingSizeTValues() {
    val style =
      JavaCppStructs.styleImageOptionsSnapshot(
        StyleImageOptions().apply {
          pixelRatio = 0.0f
          sdf = false
        }
      )
    assertNotEquals(0, style.fields)
    assertEquals(0.0f, style.pixelRatio)
    assertEquals(false, style.sdf)
    assertEquals(
      TextureImageInfo(2, 3, 8, 24),
      JavaCppStructs.textureImageInfoSnapshot(2, 3, 8, 24),
    )
    assertFailsWith<IllegalArgumentException> {
      JavaCppStructs.textureImageInfoSnapshot(2, 3, 8, -1)
    }
    assertFailsWith<IllegalArgumentException> { JavaCppStructs.styleImageInfoSnapshot(-1) }
    assertEquals(1, JavaCppStructs.styleStringListCleanupAfterCopyFailure())
    val source = JavaCppStructs.sourceInfoSnapshot(999, true)
    assertEquals(999, source.type.nativeValue)
    assertTrue(source.volatileSource)
  }

  @Test
  fun backendDescriptorsPreserveExtentsPointersAndBackendFields() {
    val extent = RenderTargetExtent(640, 480, 2.0)
    val metal =
      JavaCppStructs.metalSnapshot(
        MetalBorrowedTextureDescriptor(extent, 65, 33, NativePointer.ofAddress(0x20))
      )
    assertEquals(640, metal.width)
    assertEquals(0x20, metal.firstPointer)

    val context =
      VulkanContextDescriptor(
        NativePointer.ofAddress(0x10),
        NativePointer.ofAddress(0x20),
        NativePointer.ofAddress(0x30),
        NativePointer.ofAddress(0x40),
        7,
        NativePointer.ofAddress(0x41),
        NativePointer.ofAddress(0x42),
      )
    val vulkan =
      JavaCppStructs.vulkanSnapshot(
        VulkanBorrowedTextureDescriptor(
            extent,
            65,
            33,
            context,
            NativePointer.ofAddress(0x50),
            NativePointer.ofAddress(0x60),
            44,
            1,
          )
          .apply { finalLayout = 2 }
      )
    assertEquals(0x50, vulkan.firstPointer)
    assertEquals(0x60, vulkan.secondPointer)
    assertEquals(2, vulkan.extra)

    val egl =
      EglContextDescriptor(
        NativePointer.ofAddress(0x70),
        NativePointer.ofAddress(0x71),
        NativePointer.ofAddress(0x72),
        NativePointer.ofAddress(0x73),
      )
    val openGl =
      JavaCppStructs.openGlSnapshot(OpenGLBorrowedTextureDescriptor(extent, 65, 33, egl, 17, 3553))
    assertEquals(17, openGl.firstPointer)
    assertEquals(0x70, openGl.secondPointer)
    assertEquals(3553, openGl.extra)
  }

  @Test
  fun mapAndResourceDescriptorsCopyAllCreationFields() {
    val map =
      JavaCppStructs.mapOptionsSnapshot(
        MapOptions().apply {
          width = 320
          height = 240
          scaleFactor = 2.0
          mapMode = MapMode.STATIC
          fastPforEnabled = true
        }
      )
    assertEquals(320, map.width)
    assertEquals(240, map.height)
    assertEquals(MapMode.STATIC.nativeValue, map.mapMode)
    assertTrue(map.fastPforEnabled)

    val resource =
      JavaCppStructs.resourceResponseSnapshot(
        ResourceResponse(ResourceResponseStatus.OK).apply {
          bytes = byteArrayOf(1, 2, 3)
          mustRevalidate = true
          modifiedUnixMs = 10
          expiresUnixMs = 20
          retryAfterUnixMs = 30
        }
      )
    assertEquals(ResourceResponseStatus.OK.nativeValue, resource.status)
    assertContentEquals(byteArrayOf(1, 2, 3), resource.bytes)
    assertTrue(resource.mustRevalidate)
    assertTrue(resource.hasModified)
    assertTrue(resource.hasExpires)
    assertTrue(resource.hasRetryAfter)
  }
}
