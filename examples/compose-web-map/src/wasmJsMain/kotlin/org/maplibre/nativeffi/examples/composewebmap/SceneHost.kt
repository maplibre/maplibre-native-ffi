@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.examples.composewebmap

import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.InternalComposeUiApi
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.asComposeCanvas
import androidx.compose.ui.input.pointer.PointerButtons
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.PointerKeyboardModifiers
import androidx.compose.ui.scene.CanvasLayersComposeScene
import androidx.compose.ui.scene.ComposeScene
import androidx.compose.ui.unit.Density
import androidx.compose.ui.unit.IntSize
import kotlin.math.ceil
import kotlinx.browser.document
import kotlinx.browser.window
import kotlinx.coroutines.Dispatchers
import org.jetbrains.skia.DirectContext
import org.jetbrains.skia.Surface
import org.jetbrains.skiko.SkiaLayer
import org.jetbrains.skiko.SkikoRenderDelegate
import org.w3c.dom.HTMLCanvasElement
import org.w3c.dom.HTMLElement
import org.w3c.dom.events.Event
import org.w3c.dom.events.WheelEvent
import org.w3c.dom.pointerevents.PointerEvent

// Surface.recordingContext creates an owning wrapper in the Skiko version
// pinned by Compose 1.11. Retaining it for the page lifetime prevents that
// borrowed context from being finalized.
internal var composeDirectContext: DirectContext? = null
internal var composeFrameTick by mutableIntStateOf(0)
internal var composeFrameProducer: (() -> Unit)? = null

@OptIn(InternalComposeUiApi::class)
internal fun startComposeScene(containerId: String, content: @Composable () -> Unit) {
  val container =
    checkNotNull(document.getElementById(containerId) as? HTMLElement) {
      "Compose scene container #$containerId is unavailable"
    }
  val canvas = document.createElement("canvas") as HTMLCanvasElement
  canvas.style.apply {
    width = "100%"
    height = "100%"
    outline = "none"
    setProperty("touch-action", "none")
  }
  canvas.tabIndex = 0
  container.appendChild(canvas)

  val density = Density(window.devicePixelRatio.toFloat())
  lateinit var layer: SkiaLayer
  val scene: ComposeScene =
    CanvasLayersComposeScene(
      density = density,
      coroutineContext = Dispatchers.Main,
      invalidate = { layer.needRender() },
    )
  layer =
    SkiaLayer().apply {
      renderDelegate = SkikoRenderDelegate { rootCanvas, _, _, nanoTime ->
        recordComposeRender()
        retainComposeDirectContext(rootCanvas)
        scene.render(rootCanvas.asComposeCanvas(), nanoTime)
      }
    }

  fun resize() {
    val physicalWidth = ceil(container.clientWidth * density.density).toInt()
    val physicalHeight = ceil(container.clientHeight * density.density).toInt()
    canvas.width = physicalWidth
    canvas.height = physicalHeight
    scene.size = IntSize(physicalWidth, physicalHeight)
    layer.attachTo(canvas)
    layer.needRender()
  }

  installPointerEvents(canvas, scene, density)
  window.addEventListener("resize", { resize() })
  scene.setContent(content)
  resize()

  // MapLibre's worker events become render requests only when renderFrame()
  // polls them. Keep the prototype scene ticking even when the surrounding
  // Compose hierarchy is otherwise static.
  lateinit var renderLoop: (Double) -> Unit
  renderLoop = {
    recordComposeFramePump()
    // Keep MapLibre's WebGPU submissions outside Skia's WebGL render
    // callback. The following Compose frame consumes the completed bridge
    // image and wraps its WebGL texture while Skia's context is current.
    composeFrameProducer?.invoke()
    composeFrameTick++
    layer.needRender()
    window.requestAnimationFrame(renderLoop)
  }
  window.requestAnimationFrame(renderLoop)
}

@Suppress("INVISIBLE_MEMBER", "INVISIBLE_REFERENCE")
private fun retainComposeDirectContext(canvas: org.jetbrains.skia.Canvas) {
  if (composeDirectContext != null) return
  val surface = canvas._owner as? Surface
  composeDirectContext =
    checkNotNull(surface?.recordingContext) {
      "Compose root Skia surface has no GPU recording context"
    }
}

@OptIn(ExperimentalComposeUiApi::class, InternalComposeUiApi::class)
private fun installPointerEvents(canvas: HTMLCanvasElement, scene: ComposeScene, density: Density) {
  fun sendPointer(event: PointerEvent, type: PointerEventType) {
    scene.sendPointerEvent(
      eventType = type,
      position =
        Offset(
          event.offsetX.toFloat() * density.density,
          event.offsetY.toFloat() * density.density,
        ),
      timeMillis = event.timeStamp.toInt().toLong(),
      buttons =
        PointerButtons(
          isPrimaryPressed = event.buttons.toInt() and 1 != 0,
          isSecondaryPressed = event.buttons.toInt() and 2 != 0,
          isTertiaryPressed = event.buttons.toInt() and 4 != 0,
        ),
      keyboardModifiers =
        PointerKeyboardModifiers(
          isCtrlPressed = event.ctrlKey,
          isMetaPressed = event.metaKey,
          isAltPressed = event.altKey,
          isShiftPressed = event.shiftKey,
        ),
      nativeEvent = event,
    )
    if (event.cancelable) event.preventDefault()
  }

  fun pointerListener(type: PointerEventType): (Event) -> Unit = { event ->
    sendPointer(event as PointerEvent, type)
  }

  canvas.addEventListener("pointerdown", pointerListener(PointerEventType.Press))
  canvas.addEventListener("pointermove", pointerListener(PointerEventType.Move))
  canvas.addEventListener("pointerup", pointerListener(PointerEventType.Release))
  canvas.addEventListener("pointercancel", pointerListener(PointerEventType.Release))
  canvas.addEventListener("pointerenter", pointerListener(PointerEventType.Enter))
  canvas.addEventListener("pointerleave", pointerListener(PointerEventType.Exit))
  canvas.addEventListener(
    "wheel",
    { rawEvent ->
      val event = rawEvent as WheelEvent
      scene.sendPointerEvent(
        eventType = PointerEventType.Scroll,
        position =
          Offset(
            event.offsetX.toFloat() * density.density,
            event.offsetY.toFloat() * density.density,
          ),
        scrollDelta =
          Offset(
            normalizedWheelDelta(event.deltaX, event.deltaMode),
            normalizedWheelDelta(event.deltaY, event.deltaMode),
          ),
        timeMillis = event.timeStamp.toInt().toLong(),
        keyboardModifiers =
          PointerKeyboardModifiers(
            isCtrlPressed = event.ctrlKey,
            isMetaPressed = event.metaKey,
            isAltPressed = event.altKey,
            isShiftPressed = event.shiftKey,
          ),
        nativeEvent = event,
      )
      if (event.cancelable) event.preventDefault()
    },
  )
}

private fun normalizedWheelDelta(delta: Double, mode: Int): Float {
  // Compose's desktop pointer API reports wheel notches, while DOM pixel-mode
  // wheel events are commonly around 100 pixels per notch.
  val scale =
    when (mode) {
      0 -> 0.01
      1 -> 1.0
      2 -> 3.0
      else -> 1.0
    }
  return (delta * scale).toFloat()
}
