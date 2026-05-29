package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runtime.NetworkStatus

class MaplibreTest {
  @Test
  fun processGlobalNetworkStatusAndProjectionHelpersMatchNativeAbi() {
    Maplibre.loadNativeLibrary()
    Maplibre.cVersion()
    Maplibre.supportedRenderBackends()

    val original = Maplibre.networkStatus
    try {
      Maplibre.networkStatus = NetworkStatus.OFFLINE
      assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus)
      Maplibre.networkStatus = NetworkStatus.ONLINE
      assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus)
    } finally {
      Maplibre.networkStatus = original
    }

    val meters = Maplibre.projectedMetersForLatLng(LatLng(0.0, 0.0))
    val coordinate = Maplibre.latLngForProjectedMeters(meters)
    assertEquals(0.0, coordinate.latitude)
    assertEquals(0.0, coordinate.longitude)
  }

  @Test
  fun animationOptionsUseNullableDurationMsShape() {
    val options = AnimationOptions().apply { durationMs = 12.0 }
    assertEquals(12.0, options.durationMs)
    options.durationMs = null
    assertEquals(null, options.durationMs)
  }
}
