package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.UnitBezier
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.MLN_ANIMATION_OPTION_DURATION
import org.maplibre.nativeffi.internal.c.MLN_ANIMATION_OPTION_EASING
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ANCHOR
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_CENTER
import org.maplibre.nativeffi.internal.c.MLN_CAMERA_OPTION_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_MAP_TILE_OPTION_LOD_MODE
import org.maplibre.nativeffi.internal.c.MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE
import org.maplibre.nativeffi.internal.c.MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION
import org.maplibre.nativeffi.internal.c.MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
import org.maplibre.nativeffi.internal.c.MLN_PROJECTION_MODE_AXONOMETRIC
import org.maplibre.nativeffi.internal.c.MLN_PROJECTION_MODE_X_SKEW
import org.maplibre.nativeffi.internal.c.MLN_PROJECTION_MODE_Y_SKEW
import org.maplibre.nativeffi.internal.c.mln_animation_options
import org.maplibre.nativeffi.internal.c.mln_camera_options
import org.maplibre.nativeffi.internal.c.mln_map_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options
import org.maplibre.nativeffi.internal.c.mln_projection_mode
import org.maplibre.nativeffi.map.ConstrainMode
import org.maplibre.nativeffi.map.NorthOrientation
import org.maplibre.nativeffi.map.ProjectionModeOptions
import org.maplibre.nativeffi.map.TileLodMode
import org.maplibre.nativeffi.map.TileOptions
import org.maplibre.nativeffi.map.ViewportMode
import org.maplibre.nativeffi.map.ViewportOptions

@OptIn(ExperimentalForeignApi::class)
class MapStructsTest {
  // BND-060, BND-061: representative input structs initialize defaults and optional field masks.

  @Test
  fun cameraAndAnimationOptionsInitializeDefaultsAndPresentZeroMasks() {
    memScoped {
      val absentCamera = MapStructs.cameraOptions(CameraOptions(), this).pointed
      assertEquals(sizeOf<mln_camera_options>().toUInt(), absentCamera.size)
      assertEquals(0U, absentCamera.fields)

      val camera =
        MapStructs.cameraOptions(
            CameraOptions().apply {
              center = LatLng(0.0, 0.0)
              zoom = 0.0
              anchor = ScreenPoint(0.0, 0.0)
            },
            this,
          )
          .pointed

      assertEquals(sizeOf<mln_camera_options>().toUInt(), camera.size)
      assertEquals(
        MLN_CAMERA_OPTION_CENTER or MLN_CAMERA_OPTION_ANCHOR or MLN_CAMERA_OPTION_ZOOM,
        camera.fields,
      )
      assertEquals(0.0, camera.latitude)
      assertEquals(0.0, camera.longitude)
      assertEquals(0.0, camera.zoom)
      assertEquals(0.0, camera.anchor.x)
      assertEquals(0.0, camera.anchor.y)

      val animation =
        MapStructs.animationOptions(
            AnimationOptions().apply {
              durationMs = 0.0
              easing = UnitBezier(0.0, 0.0, 1.0, 1.0)
            },
            this,
          )
          .pointed

      assertEquals(sizeOf<mln_animation_options>().toUInt(), animation.size)
      assertEquals(MLN_ANIMATION_OPTION_DURATION or MLN_ANIMATION_OPTION_EASING, animation.fields)
      assertEquals(0.0, animation.duration_ms)
      assertEquals(0.0, animation.easing.x1)
      assertEquals(1.0, animation.easing.x2)
    }
  }

  @Test
  fun cameraOptionsRoundTripOnlyPresentFields() {
    val camera =
      CameraOptions().apply {
        center = LatLng(12.0, 34.0)
        zoom = 5.0
        padding = EdgeInsets(1.0, 2.0, 3.0, 4.0)
        anchor = ScreenPoint(8.0, 9.0)
      }

    val copy = memScoped {
      MapStructs.cameraOptions(MapStructs.cameraOptions(camera, this).pointed)
    }

    assertNotNull(copy.center)
    assertEquals(12.0, copy.center?.latitude)
    assertEquals(34.0, copy.center?.longitude)
    assertNotNull(copy.zoom)
    assertEquals(5.0, copy.zoom)
    assertNotNull(copy.padding)
    assertEquals(EdgeInsets(1.0, 2.0, 3.0, 4.0), copy.padding)
    assertNotNull(copy.anchor)
    assertEquals(ScreenPoint(8.0, 9.0), copy.anchor)
    assertFalse(copy.bearing != null)
  }

  @Test
  fun viewportAndTileOptionEnumSettersStorePublicValues() {
    val viewport =
      ViewportOptions().apply {
        northOrientation = NorthOrientation.LEFT
        constrainMode = ConstrainMode.SCREEN
        viewportMode = ViewportMode.FLIPPED_Y
      }
    val tile = TileOptions().apply { lodMode = TileLodMode.DISTANCE }

    assertEquals(NorthOrientation.LEFT, viewport.northOrientation)
    assertEquals(ConstrainMode.SCREEN, viewport.constrainMode)
    assertEquals(ViewportMode.FLIPPED_Y, viewport.viewportMode)
    assertEquals(TileLodMode.DISTANCE, tile.lodMode)
  }

  @Test
  fun viewportAndTileOptionSnapshotsPreserveUnknownEnumRawValues() {
    memScoped {
      val viewport = alloc<mln_map_viewport_options>()
      viewport.size = sizeOf<mln_map_viewport_options>().toUInt()
      viewport.fields =
        MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION or
          MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE or
          MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE
      viewport.north_orientation = 900U
      viewport.constrain_mode = 901U
      viewport.viewport_mode = 902U
      val tile = alloc<mln_map_tile_options>()
      tile.size = sizeOf<mln_map_tile_options>().toUInt()
      tile.fields = MLN_MAP_TILE_OPTION_LOD_MODE
      tile.lod_mode = 903U

      val viewportCopy = MapStructs.viewportOptions(viewport)
      val tileCopy = MapStructs.tileOptions(tile)

      assertEquals(NorthOrientation(900), viewportCopy.northOrientation)
      assertEquals(ConstrainMode(901), viewportCopy.constrainMode)
      assertEquals(ViewportMode(902), viewportCopy.viewportMode)
      assertEquals(TileLodMode(903), tileCopy.lodMode)
    }
  }

  @Test
  fun projectionModeOptionsInitializeDefaultsAndPresentZeroMasks() {
    memScoped {
      val absentProjection = MapStructs.projectionModeOptions(ProjectionModeOptions(), this).pointed
      assertEquals(sizeOf<mln_projection_mode>().toUInt(), absentProjection.size)
      assertEquals(0U, absentProjection.fields)

      val projection =
        MapStructs.projectionModeOptions(
            ProjectionModeOptions().apply {
              axonometric = false
              xSkew = 0.0
              ySkew = 0.0
            },
            this,
          )
          .pointed

      assertEquals(sizeOf<mln_projection_mode>().toUInt(), projection.size)
      assertEquals(
        MLN_PROJECTION_MODE_AXONOMETRIC or MLN_PROJECTION_MODE_X_SKEW or MLN_PROJECTION_MODE_Y_SKEW,
        projection.fields,
      )
      assertEquals(false, projection.axonometric)
      assertEquals(0.0, projection.x_skew)
      assertEquals(0.0, projection.y_skew)

      val copy = MapStructs.projectionModeOptions(projection)
      assertEquals(false, copy.axonometric)
      assertEquals(0.0, copy.xSkew)
      assertEquals(0.0, copy.ySkew)
    }
  }
}
