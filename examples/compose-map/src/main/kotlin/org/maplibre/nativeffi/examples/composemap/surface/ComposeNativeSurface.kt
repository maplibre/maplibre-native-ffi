package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import javax.swing.SwingUtilities

@Composable
public fun ComposeNativeSurface(
  renderer: NativeSurfaceRenderer,
  modifier: Modifier = Modifier,
  controller: NativeSurfaceController? = null,
) {
  val internalController = rememberNativeSurfaceController()
  val activeController = controller ?: internalController
  val density = LocalDensity.current
  val drawState = remember { NativeSurfaceDrawState() }
  var extent by remember { mutableStateOf(SurfaceExtent.Empty) }
  var frameRequest by remember { mutableLongStateOf(0L) }
  val frameSignal = frameRequest
  val bridgeSelection =
    remember(renderer, activeController) { NativeSurfaceBridge.select(renderer.backend) }
  val bridge = (bridgeSelection as? NativeSurfaceBridgeSelection.Selected)?.bridge
  val session =
    remember(bridge, activeController) {
      bridge?.let { NativeSurfaceSessionImpl(it, activeController) }
    }
  val nativeSurfaceState by activeController.state.collectAsState()
  val surfaceReady = nativeSurfaceState is NativeSurfaceState.Ready

  DisposableEffect(renderer) {
    val participant = DesktopNativeRenderingLifecycle.register { renderer.close() }
    onDispose { participant.close() }
  }

  DisposableEffect(renderer, bridgeSelection, bridge, session, activeController) {
    when (bridgeSelection) {
      is NativeSurfaceBridgeSelection.Failed -> {
        activeController.setState(
          NativeSurfaceState.Failed(
            message = "Native surface bridge initialization failed: ${bridgeSelection.message}",
            cause = bridgeSelection.error,
          )
        )
        onDispose { activeController.setState(NativeSurfaceState.Inactive) }
      }
      NativeSurfaceBridgeSelection.Unsupported -> {
        activeController.setState(
          NativeSurfaceState.Unsupported(
            requestedBackend = renderer.backend,
            host = NativeSurfaceBridge.host,
          )
        )
        onDispose { activeController.setState(NativeSurfaceState.Inactive) }
      }
      is NativeSurfaceBridgeSelection.Selected -> {
        check(bridge != null && session != null) { "Selected native surface bridge is not ready" }
        val participant = DesktopNativeRenderingLifecycle.register {
          try {
            renderer.onSurfaceLost()
          } finally {
            bridge.close()
          }
        }
        activeController.connect(
          onRequestFrame = { frameRequest += 1 },
          onDispose = participant::close,
        )
        activeController.setState(NativeSurfaceState.Ready(bridge.backend, bridge.capabilities))
        renderer.onSurfaceAvailable(session)
        session.requestFrame()
        onDispose {
          try {
            participant.close()
          } finally {
            activeController.disconnect()
            activeController.setState(NativeSurfaceState.Inactive)
          }
        }
      }
    }
  }

  LaunchedEffect(extent, bridge, renderer, surfaceReady) {
    if (surfaceReady && !extent.isEmpty && bridge != null) {
      try {
        extent.log("compose viewport")
        bridge.resize(extent)
        drawState.resetForExtent(extent)
        renderer.onSurfaceChanged(extent)
        session?.requestFrame()
      } catch (error: Throwable) {
        error.printStackTrace()
        activeController.setState(
          NativeSurfaceState.Failed(
            message =
              "Native surface bridge failed to resize to ${extent.width}x${extent.height}: ${error.message}",
            cause = error,
          )
        )
      }
    }
  }

  Canvas(
    modifier =
      modifier.fillMaxSize().onSizeChanged { size ->
        extent = SurfaceExtent.fromPhysical(size.width, size.height, density.density.toDouble())
      }
  ) {
    frameSignal
    var drew = false
    if (surfaceReady && !extent.isEmpty && bridge != null && session != null) {
      val frameId = drawState.nextFrameId()
      val frame =
        try {
          bridge.acquireFrame(frameId, extent, System.nanoTime())
        } catch (error: Throwable) {
          error.printStackTrace()
          activeController.setState(
            NativeSurfaceState.Failed(
              message = "Native surface bridge failed to acquire frame $frameId: ${error.message}",
              cause = error,
            )
          )
          null
        }
      if (frame != null) {
        try {
          when (bridge.withProducerAccess(frame) { renderer.render(frame) }) {
            NativeSurfaceRenderResult.Rendered -> {
              bridge.completeProducerAccess(frame)
              drawState.lastRenderedTarget = frame.target
            }
            NativeSurfaceRenderResult.Skipped -> Unit
          }
          drawState.lastRenderedTarget?.let { target -> drew = bridge.draw(this, target) }
        } catch (error: Throwable) {
          error.printStackTrace()
          activeController.setState(
            NativeSurfaceState.Failed(
              message = "Native surface renderer failed for frame $frameId: ${error.message}",
              cause = error,
            )
          )
        } finally {
          bridge.releaseFrame(frame)
        }
      }
    }
    if (!drew) {
      drawRect(Color(0xFF101418))
    }
  }
}

private class NativeSurfaceDrawState {
  private var extent = SurfaceExtent.Empty
  private var nextFrameId = 1L

  var lastRenderedTarget: NativeSurfaceTarget? = null

  fun resetForExtent(next: SurfaceExtent) {
    if (next != extent) {
      extent = next
      lastRenderedTarget = null
    }
  }

  fun nextFrameId(): Long = nextFrameId++
}

private class NativeSurfaceSessionImpl(
  private val bridge: NativeSurfaceBridge,
  private val controller: NativeSurfaceController,
) : NativeSurfaceSession {
  override val backend: ProducerBackend = bridge.backend

  override val capabilities: NativeSurfaceCapabilities = bridge.capabilities

  override fun requestFrame() {
    if (SwingUtilities.isEventDispatchThread()) {
      controller.requestFrame()
    } else {
      SwingUtilities.invokeLater { controller.requestFrame() }
    }
  }

  override fun <T> withRendererAccess(action: () -> T): T = bridge.withRendererAccess(action)
}
