package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogSeverity
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.runtime.NetworkStatus

class PublicApiParityTest {
  @Test
  fun enumNativeMappingsStayInternalAndExplicit() {
    assertEquals(LogEvent.SETUP, LogEvent.fromNative(LogEvent.SETUP.nativeValue))
    assertEquals(LogSeverity.ERROR, LogSeverity.fromNative(LogSeverity.ERROR.nativeValue))
    assertEquals(setOf(RenderBackend.METAL), RenderBackend.fromMask(RenderBackend.METAL.nativeMask))
    assertEquals(
      setOf(OpenGLContextProvider.EGL),
      OpenGLContextProvider.fromMask(OpenGLContextProvider.EGL.nativeMask),
    )
    assertEquals(MapMode.STATIC, MapMode.fromNative(MapMode.STATIC.nativeValue))
    assertEquals(
      OfflineRegionDownloadState.ACTIVE,
      OfflineRegionDownloadState.fromNative(OfflineRegionDownloadState.ACTIVE.nativeValue),
    )
    assertEquals(RenderMode.FULL, RenderMode.fromNative(RenderMode.FULL.nativeValue))
    assertEquals(ResourceKind.STYLE, ResourceKind.fromNative(ResourceKind.STYLE.nativeValue))
    assertEquals(NetworkStatus.ONLINE, NetworkStatus.fromNative(NetworkStatus.ONLINE.nativeValue))
  }
}
