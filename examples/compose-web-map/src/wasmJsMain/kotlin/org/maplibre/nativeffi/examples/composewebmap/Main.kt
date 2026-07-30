@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.examples.composewebmap

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.Button
import androidx.compose.material.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
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
import androidx.compose.ui.unit.sp
import kotlin.js.JsAny
import kotlin.math.ceil
import kotlin.math.max
import kotlin.math.pow
import kotlinx.coroutines.await
import org.jetbrains.skia.BackendTexture
import org.jetbrains.skia.ColorType
import org.jetbrains.skia.Image
import org.jetbrains.skia.Rect
import org.jetbrains.skia.SamplingMode
import org.jetbrains.skia.SurfaceOrigin

@OptIn(ExperimentalComposeUiApi::class)
fun main() {
  startComposeScene("composeApp") { PrototypeApp() }
}

@Composable
private fun PrototypeApp() {
  var modifiersEnabled by remember { mutableStateOf(true) }
  Box(Modifier.fillMaxSize().background(Color(0xFF071019)).padding(44.dp)) {
    Column {
      Text("MapLibre Native WebGPU · Compose/Wasm", color = Color.White)
      Spacer(Modifier.height(7.dp))
      Text(
        "The map is a Skia image in the Compose scene—not a DOM canvas overlay.",
        color = Color(0xFF9FB4C8),
      )
      Spacer(Modifier.height(12.dp))
      Button(onClick = { modifiersEnabled = !modifiersEnabled }) {
        Text(if (modifiersEnabled) "Turn map modifiers off" else "Turn map modifiers on")
      }
      Spacer(Modifier.height(16.dp))
      CompositionProof(modifiersEnabled)
    }
  }
}

@Composable
private fun CompositionProof(modifiersEnabled: Boolean) {
  val shape =
    if (modifiersEnabled) {
      RoundedCornerShape(34.dp)
    } else {
      RoundedCornerShape(0.dp)
    }
  Box(Modifier.size(820.dp, 540.dp)) {
    Canvas(Modifier.fillMaxSize()) {
      val spacing = 34.dp.toPx()
      var x = -size.height
      while (x < size.width) {
        drawLine(
          color = Color(0x557F5AF0),
          start = Offset(x, size.height),
          end = Offset(x + size.height, 0f),
          strokeWidth = 2.dp.toPx(),
        )
        x += spacing
      }
    }
    Text(
      "COMPOSE UI · BELOW MAP TEXTURE",
      color = Color(0xFFB794F4),
      fontSize = 44.sp,
      modifier =
        Modifier.align(Alignment.Center)
          .graphicsLayer(rotationZ = 7f)
          .background(Color(0xCC29145C), RoundedCornerShape(14.dp))
          .padding(horizontal = 20.dp, vertical = 12.dp),
    )
    MapLibreWebGpuMap(
      Modifier.align(Alignment.Center)
        .size(760.dp, 500.dp)
        .graphicsLayer(
          rotationZ = if (modifiersEnabled) -2.5f else 0f,
          alpha = if (modifiersEnabled) 0.86f else 1f,
          scaleX = if (modifiersEnabled) 0.985f else 1f,
          scaleY = if (modifiersEnabled) 0.985f else 1f,
        )
        .shadow(if (modifiersEnabled) 25.dp else 0.dp, shape)
        .clip(shape)
        .border(if (modifiersEnabled) 3.dp else 0.dp, Color(0xFF69D2E7), shape)
    )
    Text(
      "COMPOSE UI · ABOVE MAP TEXTURE",
      color = Color.White,
      modifier =
        Modifier.align(Alignment.BottomEnd)
          .graphicsLayer(rotationZ = 1.5f)
          .shadow(12.dp, RoundedCornerShape(12.dp))
          .background(Color(0xEE7F1D7A), RoundedCornerShape(12.dp))
          .border(2.dp, Color(0xFFFF8FE5), RoundedCornerShape(12.dp))
          .padding(horizontal = 14.dp, vertical = 10.dp),
    )
  }
}

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun MapLibreWebGpuMap(modifier: Modifier = Modifier) {
  val density = LocalDensity.current.density.toDouble()
  var extent by remember { mutableStateOf(MapExtent.Empty) }
  var nativeMap by remember { mutableStateOf<NativeMap?>(null) }
  var failure by remember { mutableStateOf<String?>(null) }
  var lastPointer by remember { mutableStateOf<Offset?>(null) }

  LaunchedEffect(extent) {
    if (extent == MapExtent.Empty) return@LaunchedEffect
    runCatching {
        val previous = nativeMap
        val module = previous?.module ?: createMapLibreModule(mapLibreModuleOptions()).await()
        val staging = previous?.staging ?: createWebGpuStagingBridge(module).await()
        val composeCanvas = checkNotNull(composeCanvas()) { "Compose canvas is unavailable" }
        val webGl = checkNotNull(webGl2Context(composeCanvas)) { "Compose WebGL2 is unavailable" }
        val texture = createComposeMapTexture(webGl, extent.physicalWidth, extent.physicalHeight)

        val status =
          if (previous == null) {
            staging.resize(extent.physicalWidth, extent.physicalHeight)
            val target = staging.borrowTarget()
            module
              .initBorrowed(
                extent.width,
                extent.height,
                extent.scale,
                -122.4194,
                37.7749,
                13.0,
                12.0,
                30.0,
                staging.devicePointer,
                0,
                target.texturePointer,
                target.textureViewPointer,
                target.textureFormat,
              )
              .also { initStatus ->
                if (initStatus == 0) {
                  check(module.clearBorrowedTarget() == 0) {
                    "MapLibre initial WebGPU borrowed target release failed"
                  }
                }
                staging.finishTarget(false)
              }
          } else {
            val resizeStatus = module.resize(extent.width, extent.height, extent.scale)
            if (resizeStatus == 0) {
              staging.resize(extent.physicalWidth, extent.physicalHeight)
            }
            resizeStatus
          }
        check(status == 0) { "MapLibre resize/init returned status $status" }
        nativeMap = NativeMap(module, staging, webGl, texture, extent)
        failure = null
      }
      .onFailure { failure = it.stackTraceToString() }
  }

  DisposableEffect(nativeMap) {
    val current = nativeMap
    composeFrameProducer = current?.let { { it.produceMapFrame() } }
    onDispose {
      composeFrameProducer = null
      current?.closeComposeImage()
    }
  }

  Box(modifier.background(Color(0xFF101820))) {
    Canvas(
      Modifier.fillMaxSize()
        .onSizeChanged { extent = MapExtent.fromPhysical(it.width, it.height, density) }
        .onPointerEvent(PointerEventType.Press) { event ->
          nativeMap?.module?.cancelTransitions()
          lastPointer = event.changes.firstOrNull()?.position
        }
        .onPointerEvent(PointerEventType.Move) { event ->
          val change = event.changes.firstOrNull() ?: return@onPointerEvent
          val previous = lastPointer ?: change.position
          if (change.pressed) {
            val delta = change.position - previous
            nativeMap?.module?.moveBy(delta.x / density, delta.y / density)
            change.consume()
          }
          lastPointer = change.position
        }
        .onPointerEvent(PointerEventType.Release) { lastPointer = null }
        .onPointerEvent(PointerEventType.Scroll) { event ->
          val change = event.changes.firstOrNull() ?: return@onPointerEvent
          nativeMap
            ?.module
            ?.scaleBy(
              2.0.pow(-change.scrollDelta.y * 0.25),
              change.position.x / density,
              change.position.y / density,
            )
          change.consume()
        }
    ) {
      composeFrameTick
      nativeMap?.let { current ->
        current.consumeFrame()
        drawIntoCanvas { canvas ->
          val image =
            current.image ?: if (current.hasUploadedFrame) current.createComposeImage() else null
          if (image == null) return@drawIntoCanvas
          canvas.skiaCanvas.drawImageRect(
            image,
            Rect.makeWH(image.width.toFloat(), image.height.toFloat()),
            Rect.makeWH(size.width, size.height),
            SamplingMode.LINEAR,
            null,
            true,
          )
        }
      }
    }

    Row(
      Modifier.padding(18.dp)
        .background(Color(0xD9101820), RoundedCornerShape(14.dp))
        .padding(horizontal = 12.dp, vertical = 8.dp)
    ) {
      Text("WEBGPU", color = Color(0xFF69D2E7))
      Spacer(Modifier.width(10.dp))
      Text("ONE GPU COPY · Compose scene content", color = Color.White)
    }

    failure?.let {
      Text(
        it,
        color = Color(0xFFFF6B6B),
        modifier =
          Modifier.padding(18.dp)
            .background(Color(0xE0201010), RoundedCornerShape(12.dp))
            .padding(12.dp),
      )
    }
  }
}

private data class MapExtent(
  val width: Int,
  val height: Int,
  val scale: Double,
  val physicalWidth: Int,
  val physicalHeight: Int,
) {
  companion object {
    val Empty = MapExtent(0, 0, 1.0, 0, 0)

    fun fromPhysical(width: Int, height: Int, scale: Double): MapExtent {
      val logicalWidth = max(1, ceil(width / scale).toInt())
      val logicalHeight = max(1, ceil(height / scale).toInt())
      return MapExtent(
        logicalWidth,
        logicalHeight,
        scale,
        ceil(logicalWidth * scale).toInt(),
        ceil(logicalHeight * scale).toInt(),
      )
    }
  }
}

private class NativeMap(
  val module: MapLibreModule,
  val staging: WebGpuStagingBridge,
  private val webGl: JsAny,
  private var texture: JsAny,
  private val extent: MapExtent,
) {
  var image: Image? = null
  private var backendTexture: BackendTexture? = null
  var hasUploadedFrame = false
    private set

  fun consumeFrame() {
    if (staging.ready) {
      val nextTexture =
        if (hasUploadedFrame) {
          createComposeMapTexture(webGl, extent.physicalWidth, extent.physicalHeight)
        } else {
          texture
        }
      if (staging.uploadToWebGl(webGl, nextTexture)) {
        composeDirectContext?.resetGLAll()
        if (hasUploadedFrame) {
          closeComposeImage()
          texture = nextTexture
        } else {
          hasUploadedFrame = true
        }
      }
    }
  }

  fun produceMapFrame() {
    if (staging.ready) return
    check(!staging.inFlight) { "WebGPU target escaped its browser task" }
    val target = staging.borrowTarget()
    check(
      module.setBorrowedTarget(
        target.texturePointer,
        target.textureViewPointer,
        target.textureFormat,
      ) == 0
    ) {
      "MapLibre WebGPU borrowed target replacement failed"
    }
    val rendered = module.renderFrame() == 1
    check(module.clearBorrowedTarget() == 0) { "MapLibre WebGPU borrowed target release failed" }
    staging.finishTarget(rendered)
  }

  fun createComposeImage(): Image {
    val context = checkNotNull(composeDirectContext) { "Compose root GPU context is unavailable" }
    context.resetGLAll()
    val skikoTexture = pushSkikoTexture(skikoGl, texture)
    backendTexture =
      BackendTexture.makeGL(
        extent.physicalWidth,
        extent.physicalHeight,
        false,
        skikoTexture,
        GL_TEXTURE_2D,
        GL_RGBA8,
      )
    return Image.adoptTextureFrom(
        context,
        checkNotNull(backendTexture),
        SurfaceOrigin.TOP_LEFT,
        ColorType.RGBA_8888,
      )
      .also { image = it }
  }

  fun closeComposeImage() {
    image?.close()
    image = null
    backendTexture?.close()
    backendTexture = null
  }

  private companion object {
    const val GL_TEXTURE_2D = 0x0DE1
    const val GL_RGBA8 = 0x8058
  }
}
