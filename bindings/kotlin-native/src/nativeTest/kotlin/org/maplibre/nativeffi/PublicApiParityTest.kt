package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertTrue
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogSeverity
import org.maplibre.nativeffi.map.DebugOption
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.runtime.NetworkStatus
import org.maplibre.nativeffi.style.TileScheme

class PublicApiParityTest {
  @Test
  fun enumNativeAccessorsArePublicKotlinValues() {
    assertTrue(LogEvent.SETUP.nativeValue > 0U)
    assertTrue(LogSeverity.ERROR.nativeValue > 0U)
    assertTrue(LogSeverity.ERROR.nativeMask > 0U)
    assertTrue(DebugOption.TILE_BORDERS.nativeMask > 0U)
    assertTrue(MapMode.STATIC.nativeValue > 0U)
    assertTrue(OfflineRegionDownloadState.ACTIVE.nativeValue > 0U)
    assertTrue(RenderBackend.METAL.nativeMask > 0U)
    assertTrue(RenderMode.FULL.nativeValue > 0U)
    assertTrue(ResourceKind.STYLE.nativeValue > 0U)
    assertTrue(NetworkStatus.ONLINE.nativeValue > 0U)
    assertTrue(TileScheme.TMS.nativeValue > 0U)
  }
}
