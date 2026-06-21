package org.maplibre.nativeffi.examples.composemap.app

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import org.maplibre.nativeffi.examples.composemap.map.MapLibreSurfaceRenderer
import org.maplibre.nativeffi.examples.composemap.surface.ComposeNativeSurface
import org.maplibre.nativeffi.examples.composemap.surface.NativeSurfaceState
import org.maplibre.nativeffi.examples.composemap.surface.rememberNativeSurfaceController

@Composable
internal fun ComposeMapApp() {
  val controller = rememberNativeSurfaceController()
  val renderer = remember { MapLibreSurfaceRenderer() }
  val state by controller.state.collectAsState()

  DisposableEffect(renderer) { onDispose { renderer.close() } }

  Box(Modifier.fillMaxSize().background(Color.Black)) {
    ComposeNativeSurface(
      renderer = renderer,
      modifier = Modifier.fillMaxSize(),
      controller = controller,
    )
    val diagnostic =
      when (val current = state) {
        is NativeSurfaceState.Failed -> current.message
        is NativeSurfaceState.Unsupported ->
          "Native surface unsupported for ${current.requestedBackends} on ${current.host.operatingSystem}/${current.host.consumerBackend}"
        NativeSurfaceState.Inactive,
        is NativeSurfaceState.Ready -> null
      }
    if (diagnostic != null) {
      Text(text = diagnostic, color = Color.White, modifier = Modifier.padding(12.dp))
    }
  }
}
