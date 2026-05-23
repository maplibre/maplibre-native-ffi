package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runtime.NetworkStatus

class MaplibreTest {
  @Test
  fun processGlobalNetworkStatusAndProjectionHelpersMatchNativeAbi() {
    Maplibre.cVersion()
    Maplibre.supportedRenderBackends()

    val original = Maplibre.networkStatus()
    try {
      Maplibre.setNetworkStatus(NetworkStatus.OFFLINE)
      assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus())
      Maplibre.setNetworkStatus(NetworkStatus.ONLINE)
      assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus())
    } finally {
      Maplibre.setNetworkStatus(original)
    }

    val meters = Maplibre.projectedMetersForLatLng(LatLng(0.0, 0.0))
    val coordinate = Maplibre.latLngForProjectedMeters(meters)
    assertEquals(0.0, coordinate.latitude)
    assertEquals(0.0, coordinate.longitude)
  }

  @Test
  fun animationDurationMsAliasesMatchKotlinDurationMillisNames() {
    val options = AnimationOptions().durationMs(12.0)
    assertTrue(options.hasDurationMs())
    assertTrue(options.hasDurationMillis())
    assertEquals(12.0, options.durationMillis)
    options.clearDurationMs()
    assertEquals(null, options.durationMillis)
  }
}
