package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
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
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

@Composable
public fun ComposeNativeSurface(
  renderer: NativeSurfaceRenderer,
  modifier: Modifier = Modifier,
  controller: NativeSurfaceController? = null,
) {
  val internalController = rememberNativeSurfaceControllerImpl()
  val activeController = controller ?: internalController
  val controllerImpl = activeController as? NativeSurfaceControllerImpl
  val density = LocalDensity.current
  val drawState = remember { NativeSurfaceDrawState() }
  var extent by remember { mutableStateOf(SurfaceExtent.Empty) }
  var frameRequest by remember { mutableLongStateOf(0L) }
  val frameSignal = frameRequest
  val bridge =
    remember(renderer.supportedBackends) { NativeSurfaceBridge.select(renderer.supportedBackends) }
  val session =
    remember(bridge, activeController) {
      bridge?.let { NativeSurfaceSessionImpl(it, activeController) }
    }

  DisposableEffect(renderer, bridge, session, controllerImpl) {
    if (bridge == null || session == null) {
      controllerImpl?.setState(
        NativeSurfaceState.Unsupported(
          requestedBackends = renderer.supportedBackends,
          host = NativeSurfaceBridge.host,
        )
      )
      onDispose { controllerImpl?.setState(NativeSurfaceState.Inactive) }
    } else {
      val participant = DesktopNativeRenderingLifecycle.register {
        renderer.close()
        bridge.close()
      }
      controllerImpl?.connect(
        onRequestFrame = { frameRequest += 1 },
        onDispose = participant::close,
      )
      controllerImpl?.setState(NativeSurfaceState.Ready(bridge.backend, bridge.capabilities))
      renderer.onSurfaceAvailable(session)
      session.requestFrame()
      onDispose {
        participant.close()
        controllerImpl?.disconnect()
        controllerImpl?.setState(NativeSurfaceState.Inactive)
      }
    }
  }

  LaunchedEffect(extent, bridge, renderer) {
    if (!extent.isEmpty && bridge != null) {
      try {
        extent.log("compose viewport")
        bridge.resize(extent)
        drawState.resetForExtent(extent)
        renderer.onSurfaceChanged(extent)
        session?.requestFrame()
      } catch (error: Throwable) {
        error.printStackTrace()
        controllerImpl?.setState(
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
    if (!extent.isEmpty && bridge != null && session != null) {
      val frameId = drawState.nextFrameId()
      val frame =
        try {
          bridge.acquireFrame(frameId, extent, System.nanoTime())
        } catch (error: Throwable) {
          error.printStackTrace()
          controllerImpl?.setState(
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
          controllerImpl?.setState(
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

@Composable
internal fun rememberNativeSurfaceControllerImpl(): NativeSurfaceController = remember {
  NativeSurfaceControllerImpl()
}

internal class NativeSurfaceControllerImpl : NativeSurfaceController {
  private val mutableState = MutableStateFlow<NativeSurfaceState>(NativeSurfaceState.Inactive)
  private var requestFrameCallback: (() -> Unit)? = null
  private var disposeCallback: (() -> Unit)? = null

  override val state: StateFlow<NativeSurfaceState> = mutableState

  override fun requestFrame() {
    requestFrameCallback?.invoke()
  }

  override fun dispose() {
    disposeCallback?.invoke()
    disconnect()
    setState(NativeSurfaceState.Inactive)
  }

  fun connect(onRequestFrame: () -> Unit, onDispose: () -> Unit) {
    requestFrameCallback = onRequestFrame
    disposeCallback = onDispose
  }

  fun disconnect() {
    requestFrameCallback = null
    disposeCallback = null
  }

  fun setState(state: NativeSurfaceState) {
    mutableState.value = state
  }
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
