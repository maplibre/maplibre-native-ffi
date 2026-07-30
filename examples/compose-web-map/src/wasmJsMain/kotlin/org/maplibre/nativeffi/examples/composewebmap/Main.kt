@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.examples.composewebmap

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.shadow
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.skiaCanvas
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.onPointerEvent
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import androidx.compose.ui.window.ComposeViewport
import kotlin.js.JsAny
import kotlin.math.ceil
import kotlin.math.max
import kotlin.math.pow
import kotlinx.coroutines.await
import org.jetbrains.skia.BackendTexture
import org.jetbrains.skia.ColorType
import org.jetbrains.skia.DirectContext
import org.jetbrains.skia.Image
import org.jetbrains.skia.Rect
import org.jetbrains.skia.SamplingMode
import org.jetbrains.skia.SurfaceOrigin
import org.jetbrains.skiko.ExperimentalSkikoApi

@OptIn(ExperimentalComposeUiApi::class)
fun main() {
  ComposeViewport("composeApp") { PrototypeApp() }
}

@Composable
private fun PrototypeApp() {
  Box(Modifier.fillMaxSize().background(Color(0xFF08111B)).padding(44.dp)) {
    Column {
      Text("MapLibre Native · Compose Web", color = Color.White)
      Text(
        "COMPOSE UI ABOVE · one shared WebGL texture, sampled inside the scene",
        color = Color(0xFF9FB3C8),
        modifier = Modifier.padding(top = 6.dp, bottom = 22.dp),
      )
      MapLibreCanvas(
        Modifier.fillMaxWidth(0.65f)
          .height(500.dp)
          .graphicsLayer(rotationZ = -2.5f, alpha = 0.92f)
          .shadow(24.dp, RoundedCornerShape(32.dp))
          .clip(RoundedCornerShape(32.dp))
          .border(3.dp, Color(0xFF69D2E7), RoundedCornerShape(32.dp))
      )
      Text(
        "COMPOSE UI BELOW · map modifier chain: rotate · alpha · shadow · clip · border",
        color = Color(0xFF9FB3C8),
        modifier = Modifier.padding(top = 18.dp),
      )
    }
  }
}

@OptIn(ExperimentalSkikoApi::class, ExperimentalComposeUiApi::class)
@Composable
private fun MapLibreCanvas(modifier: Modifier = Modifier) {
  val density = LocalDensity.current.density.toDouble()
  var extent by remember { mutableStateOf(Extent.Empty) }
  var native by remember { mutableStateOf<NativeMap?>(null) }
  var failure by remember { mutableStateOf<String?>(null) }
  var tick by remember { mutableIntStateOf(0) }
  var lastPointer by remember { mutableStateOf<Offset?>(null) }
  val retiredMaps = remember { ArrayDeque<RetiredMap>() }

  LaunchedEffect(extent) {
    if (extent == Extent.Empty) return@LaunchedEffect
    runCatching {
        val module = native?.module ?: createMapLibreComposeModule(mapLibreModuleOptions()).await()
        val previous = native
        val canvas = checkNotNull(composeCanvas())
        val gl = checkNotNull(webGL2Context(canvas))
        val texture = createMapTexture(gl, extent.physicalWidth, extent.physicalHeight)
        val contextId = native?.contextId ?: module.importWebGLContext(gl)
        val textureId = module.importWebGLTexture(texture)
        val next = NativeMap(module, gl, texture, contextId, textureId, extent)
        val status =
          if (native == null) {
            module.init(
              extent.width,
              extent.height,
              extent.scale,
              -122.4194,
              37.7749,
              13.0,
              12.0,
              30.0,
              contextId,
              textureId,
            )
          } else {
            module.resize(extent.width, extent.height, extent.scale, contextId, textureId)
          }
        check(status == 0) { "native map returned status $status" }
        // resize() destroys the session which borrows the old texture. Skia display lists can
        // retain an adopted image for several frames, so defer deleting the texture as well.
        previous?.module?.unregisterWebGLTexture(previous.textureId)
        previous?.let { retiredMaps.addLast(RetiredMap(it, tick)) }
        native = next
      }
      .onFailure { failure = it.stackTraceToString() }
  }

  LaunchedEffect(Unit) {
    while (true) {
      withFrameNanos { tick++ }
    }
  }

  Box(modifier.background(Color(0xFF101820))) {
    Canvas(
      Modifier.fillMaxSize()
        .onSizeChanged { extent = Extent.fromPhysical(it.width, it.height, density) }
        .onPointerEvent(PointerEventType.Press) { event ->
          native?.module?.cancelTransitions()
          lastPointer = event.changes.firstOrNull()?.position
        }
        .onPointerEvent(PointerEventType.Move) { event ->
          val change = event.changes.firstOrNull() ?: return@onPointerEvent
          val previous = lastPointer ?: change.position
          if (change.pressed) {
            val delta = change.position - previous
            native?.module?.moveBy(delta.x / density, delta.y / density)
            change.consume()
          }
          lastPointer = change.position
        }
        .onPointerEvent(PointerEventType.Release) { lastPointer = null }
        .onPointerEvent(PointerEventType.Scroll) { event ->
          val change = event.changes.firstOrNull() ?: return@onPointerEvent
          native
            ?.module
            ?.scaleBy(
              2.0.pow(-change.scrollDelta.y / 240.0),
              change.position.x / density,
              change.position.y / density,
            )
          change.consume()
        }
    ) {
      tick
      val current = native
      if (current != null) {
        current.module.renderFrame()
        drawIntoCanvas { canvas ->
          val context = current.skiaContext()
          context.resetGLAll()
          val image = current.image ?: current.createImage(context)
          canvas.skiaCanvas.drawImageRect(
            image,
            Rect.makeWH(image.width.toFloat(), image.height.toFloat()),
            Rect.makeWH(size.width, size.height),
            SamplingMode.LINEAR,
            null,
            true,
          )
          while (retiredMaps.firstOrNull()?.let { tick - it.retiredAt >= 8 } == true) {
            retiredMaps.removeFirst().map.closeImage()
          }
        }
      }
    }
    failure?.let { Text(it, color = Color.Red, modifier = Modifier.padding(12.dp)) }
    Box(
      Modifier.align(Alignment.TopEnd)
        .padding(18.dp)
        .background(Color(0xE6112A3A), RoundedCornerShape(12.dp))
        .border(1.dp, Color(0xFF69D2E7), RoundedCornerShape(12.dp))
        .padding(horizontal = 12.dp, vertical = 8.dp)
    ) {
      Text("COMPOSE OVERLAY\nabove the map texture", color = Color.White)
    }
  }
}

private data class RetiredMap(val map: NativeMap, val retiredAt: Int)

private data class Extent(
  val width: Int,
  val height: Int,
  val scale: Double,
  val physicalWidth: Int,
  val physicalHeight: Int,
) {
  companion object {
    val Empty = Extent(0, 0, 1.0, 0, 0)

    fun fromPhysical(width: Int, height: Int, scale: Double): Extent {
      val logicalWidth = max(1, ceil(width / scale).toInt())
      val logicalHeight = max(1, ceil(height / scale).toInt())
      return Extent(
        logicalWidth,
        logicalHeight,
        scale,
        ceil(logicalWidth * scale).toInt(),
        ceil(logicalHeight * scale).toInt(),
      )
    }
  }
}

@OptIn(ExperimentalSkikoApi::class)
private class NativeMap(
  val module: MapLibreModule,
  val gl: JsAny,
  val texture: JsAny,
  val contextId: Int,
  val textureId: Int,
  val extent: Extent,
) {
  var image: Image? = null
  private var backendTexture: BackendTexture? = null

  fun skiaContext(): DirectContext = sharedSkiaContext()

  fun createImage(context: org.jetbrains.skia.DirectContext): Image {
    val skikoTexture = pushSkikoTexture(texture)
    backendTexture =
      BackendTexture.makeGL(
        extent.physicalWidth,
        extent.physicalHeight,
        false,
        skikoTexture,
        0x0DE1,
        0x8058,
      )
    return Image.adoptTextureFrom(
        context,
        checkNotNull(backendTexture),
        SurfaceOrigin.BOTTOM_LEFT,
        ColorType.RGBA_8888,
      )
      .also { image = it }
  }

  fun closeImage() {
    image?.close()
    image = null
    backendTexture?.close()
    backendTexture = null
  }
}

private var capturedDirectContext: DirectContext? = null
private var capturedDirectContextPointer = 0
private val retiredDirectContexts = mutableListOf<DirectContext>()

@Suppress("INVISIBLE_MEMBER", "INVISIBLE_REFERENCE")
private fun sharedSkiaContext(): DirectContext {
  val pointer = composeSkiaDirectContextPointer()
  check(pointer != 0) { "Compose Skia DirectContext is not initialized" }
  if (pointer != capturedDirectContextPointer) {
    capturedDirectContext?.let { retiredDirectContexts.add(it) }
    // Keep these reflected, ownership-taking wrappers alive for the page lifetime;
    // Compose remains the real owner of each native DirectContext.
    capturedDirectContext = DirectContext(pointer)
    capturedDirectContextPointer = pointer
  }
  return checkNotNull(capturedDirectContext)
}
