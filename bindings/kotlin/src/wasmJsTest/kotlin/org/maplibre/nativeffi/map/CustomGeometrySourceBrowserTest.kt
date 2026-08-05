package org.maplibre.nativeffi.map

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertContains
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.PageCanvases
import org.maplibre.nativeffi.assertPresentedColor
import org.maplibre.nativeffi.backgroundStyle
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.wasm.CustomGeometryBridge
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.WebglContext
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.waitForMapEvent
import org.maplibre.nativeffi.withMap

/**
 * A custom geometry source whose tiles this page supplies, end to end.
 *
 * This is the one callback family in the binding that native invokes without waiting for an answer.
 * MapLibre asks for a tile from the worker the source's tile loader runs on; that worker cannot
 * enter the page's WebAssembly instance, so the module copies the tile id and posts it to the page,
 * where the host's callback body is. Because nothing is blocked while that body runs, the binding
 * delivers it on a stack that may reach the owner thread — so the host may answer with
 * `setCustomGeometrySourceTileData` from inside the callback, which is what shared expect/actual
 * code written for JVM, Android, or Kotlin/Native does, or record the tile and answer from a
 * `maplibreScope` afterwards.
 *
 * So the proof has to be the whole chain rather than any part of it, and it has to be both answers.
 * A tile the host was asked for and answered has to end up as pixels on a `<canvas>` the page
 * displays — read the way page code would read it — because every intermediate step can be right
 * while the geometry never reaches the screen. A fill layer over the custom source paints one
 * colour over a background of another, so the page changing colour is the source's geometry
 * arriving and nothing else.
 */
class CustomGeometrySourceBrowserTest {
  // Spec coverage: BND-124.
  @Test
  fun presentsTheTilesItsCallbackWasAskedForAndThePageSupplied(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { runtime, map ->
        val requested = mutableListOf<CanonicalTileId>()
        val cancelled = mutableListOf<CanonicalTileId>()
        val deferred =
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) {
              requested.add(tileId)
            }

            override fun cancelTile(tileId: CanonicalTileId) {
              cancelled.add(tileId)
            }
          }

        // The shared-code shape: answer the request inside the callback that made it. Failures are
        // captured rather than thrown, because nothing above a callback would catch one and the
        // test would then only see a tile that never arrived.
        val answeredInline = mutableListOf<CanonicalTileId>()
        var inlineFailure: Throwable? = null
        val inline =
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) {
              runCatching {
                  map.setCustomGeometrySourceTileData(SOURCE, tileId, worldFill())
                  answeredInline.add(tileId)
                }
                .exceptionOrNull()
                ?.let { if (inlineFailure == null) inlineFailure = it }
            }

            override fun cancelTile(tileId: CanonicalTileId) {}
          }

        val context = WebglContext.createForCanvas(PageCanvases.CUSTOM_GEOMETRY, WIDTH, HEIGHT)
        try {
          val session =
            map.attachOpenGLSurface(
              OpenGLSurfaceDescriptor(
                RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                context.descriptor(),
                // A WebGL context is already bound to its canvas, so there is no drawable to name.
                NativePointer.NULL,
              )
            )
          try {
            map.setStyleJson(backgroundStyle(BACKGROUND_COLOR))
            waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)

            map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(deferred))
            assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType(SOURCE))
            map.addStyleLayerJson(fillLayer(), "")

            // Nothing asks a custom source for a tile until a layer that uses it is being drawn, so
            // the request arrives while the map renders rather than when the source is added.
            assertTrue(
              renderUntil(runtime, session) { requested.isNotEmpty() },
              "the source never asked the page for a tile",
            )
            // The whole world at the zoom the map opens at, which is the tile the fill below
            // covers.
            assertContains(requested, CanonicalTileId(0, 0, 0))

            // The layer has a source and the source has no data, so what the page shows so far is
            // the background alone. Asserted before the answer, so the colour below is provably the
            // geometry arriving rather than a frame that was already there.
            renderUntilSettled(runtime, session)
            assertPresentedColor(
              PageCanvases.CUSTOM_GEOMETRY,
              BACKGROUND_RED,
              BACKGROUND_GREEN,
              BACKGROUND_BLUE,
            )

            // The answer, from the scope rather than from the callback. One polygon covering the
            // world fills the viewport at this zoom, so the fill colour is what the page must come
            // to show.
            for (tileId in requested.toList()) {
              map.setCustomGeometrySourceTileData(SOURCE, tileId, worldFill())
            }
            assertTrue(
              renderUntil(runtime, session) { map.isFullyLoaded },
              "the map never finished loading the tiles the page supplied",
            )
            renderUntilSettled(runtime, session)
            assertPresentedColor(PageCanvases.CUSTOM_GEOMETRY, FILL_RED, FILL_GREEN, FILL_BLUE)

            // A cancel is best-effort and may never arrive, so nothing here waits for one. What is
            // true whenever one does arrive is that it names a tile this source asked for.
            for (tileId in cancelled) assertContains(requested, tileId)

            // Removing the layer and then the source takes the geometry away again, which is the
            // other half of the claim: the fill was the source's rather than anything the style
            // held on its own. It is also what puts the page back to one colour, so the second half
            // of this test starts from a screen that provably holds no custom geometry.
            assertTrue(map.removeStyleLayer(FILL_LAYER))
            assertTrue(map.removeStyleSource(SOURCE))
            assertFalse(map.styleSourceExists(SOURCE))
            renderUntilSettled(runtime, session)
            assertPresentedColor(
              PageCanvases.CUSTOM_GEOMETRY,
              BACKGROUND_RED,
              BACKGROUND_GREEN,
              BACKGROUND_BLUE,
            )

            // The same source again, answered from inside the callback this time. This is the
            // workflow a multiplatform host writes once and runs everywhere, and the claim is that
            // it is not merely accepted here but that its geometry reaches the screen: nothing
            // between this and the assertion below supplies a tile, so the page can only come to
            // show the fill because the callback's own answer arrived.
            map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(inline))
            map.addStyleLayerJson(fillLayer(), "")
            assertTrue(
              renderUntil(runtime, session) { answeredInline.isNotEmpty() },
              "the callback never answered from inside itself: ${inlineFailure?.message}",
            )
            assertNull(inlineFailure, "answering inside the callback failed")
            assertTrue(
              renderUntil(runtime, session) { map.isFullyLoaded },
              "the map never finished loading the tiles the callback answered with",
            )
            renderUntilSettled(runtime, session)
            assertPresentedColor(PageCanvases.CUSTOM_GEOMETRY, FILL_RED, FILL_GREEN, FILL_BLUE)
          } finally {
            session.close()
          }
        } finally {
          context.close()
        }
      }
    }
  }

  /**
   * The teardown paths, with tile requests in flight.
   *
   * A notification is a copy of a tile id travelling on a page task, so it outlives the worker that
   * posted it and can arrive after the source it belongs to is gone. What the binding promises is
   * that such a notification is dropped rather than delivered to a registration that has retired —
   * and that a source added again under the same id is a new registration, not the old one coming
   * back.
   *
   * Rendered into a texture of its own rather than onto the page: nothing here is about what a
   * frame looks like, and a page canvas tolerates only about two WebGL contexts over the page's
   * lifetime.
   */
  // Spec coverage: BND-124.
  @Test
  fun dropsNotificationsForSourcesThatAreGoneAndReusesTheIdCleanly(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap(WIDTH, HEIGHT) { runtime, map ->
          val first = RecordingCallback()
          val second = RecordingCallback()
          // What the registry held before this test, so each teardown below can be asserted to have
          // put it back rather than to have reached zero by luck. A registration that outlives its
          // source is invisible from the outside — it does nothing until native calls it, and the
          // teardown is what made native calling it impossible — so this is what says it went.
          val registrationsBefore = CustomGeometryBridge.liveRegistrations
          val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
          try {
            val session =
              map.attachOpenGLOwnedTexture(
                OpenGLOwnedTextureDescriptor(
                  RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                  context.descriptor(),
                )
              )
            try {
              map.setStyleJson(backgroundStyle(BACKGROUND_COLOR))
              waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
              map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(first))
              map.addStyleLayerJson(fillLayer(), "")
              assertTrue(
                renderUntil(runtime, session) { first.tiles.isNotEmpty() },
                "the source never asked the page for a tile",
              )

              // Removed with the request still unanswered, so the tile is one MapLibre is still
              // waiting on: dropping the layer retires that tile and produces the cancels this
              // source's loader posts to the page, and the source goes before the page has run
              // them. The layer goes first because native refuses to remove a source a layer still
              // uses. From here the callback must hear nothing at all.
              assertTrue(map.removeStyleLayer(FILL_LAYER))
              assertTrue(map.removeStyleSource(SOURCE))
              first.closeEra()
              renderTurns(runtime, session, TEARDOWN_ATTEMPTS)
              assertEquals(0, first.afterEra, "a removed source's callback was still called")
              assertEquals(
                registrationsBefore,
                CustomGeometryBridge.liveRegistrations,
                "removing the source left its registration behind",
              )

              // The same id again, and a different callback. A registration is reached by a token
              // native carries back, and a token is issued once, so the source added here is a new
              // registration rather than the previous one under a familiar name.
              map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(second))
              map.addStyleLayerJson(fillLayer(), "")
              assertTrue(
                renderUntil(runtime, session) { second.tiles.isNotEmpty() },
                "the source added under the reused id never asked the page for a tile",
              )
              assertEquals(0, first.afterEra, "the replaced callback was called for the new source")

              // Closing the map with a source still registered is the last teardown path, and it is
              // the one that leaves native with no way to ask again: the map took its style, its
              // sources, and their tile loaders with it.
              //
              // The boundary is the close returning rather than it being called. Until then the
              // source is still the map's, and a notification the page runs while this stack is
              // parked on the close is one the callback is entitled to. What must not happen is a
              // delivery afterwards — and there is one to fear, because retiring the map's tiles
              // makes their loader post cancels from its own thread, which can land here after the
              // close has returned.
              session.close()
              map.close()
              second.closeEra()
              pumpTurns(runtime, TEARDOWN_ATTEMPTS)
              assertEquals(0, second.afterEra, "a closed map's source callback was still called")
              assertEquals(
                registrationsBefore,
                CustomGeometryBridge.liveRegistrations,
                "closing the map left its source's registration behind",
              )
            } finally {
              session.close()
            }
          } finally {
            context.close()
          }
        }
      }
    }

  /**
   * A style reload, which drops every source the previous style held.
   *
   * The registration behind a custom geometry source belongs to that source, so a style that
   * replaces it ends the registration too — while the tiles the previous style had are being
   * retired, which is exactly when their loader posts the cancels this must not deliver.
   *
   * A style arrives two ways and the moment differs. A style set as JSON has replaced the previous
   * one by the time the call returns, so the registrations go there. A style set by URL loads
   * later, and the only announcement it makes is the loaded event, so the registrations go when
   * that event is polled — the behaviour `RuntimeHandle.pollEvent` documents on every platform.
   * Both are asserted here, each at its own moment, because either alone would leave the other's
   * window open.
   */
  // Spec coverage: BND-124.
  @Test
  fun releasesSourcesAStyleReloadDropped(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap(WIDTH, HEIGHT) { runtime, map ->
        val callback = RecordingCallback()
        val registrationsBefore = CustomGeometryBridge.liveRegistrations
        val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
        try {
          val session =
            map.attachOpenGLOwnedTexture(
              OpenGLOwnedTextureDescriptor(
                RenderTargetExtent(WIDTH, HEIGHT, 1.0),
                context.descriptor(),
              )
            )
          try {
            map.setStyleJson(backgroundStyle(BACKGROUND_COLOR))
            waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
            map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(callback))
            map.addStyleLayerJson(fillLayer(), "")
            assertTrue(
              renderUntil(runtime, session) { callback.tiles.isNotEmpty() },
              "the source never asked the page for a tile",
            )

            map.setStyleJson(backgroundStyle(FILL_COLOR))
            callback.closeEra()
            // Asserted before anything is polled or rendered, because this is the claim about a
            // style set as JSON: the source is gone by the time the call returns, so its
            // registration is too.
            assertEquals(
              registrationsBefore,
              CustomGeometryBridge.liveRegistrations,
              "setting a style as JSON left the dropped source's registration behind",
            )
            assertFalse(map.styleSourceExists(SOURCE))
            renderTurns(runtime, session, TEARDOWN_ATTEMPTS)
            assertEquals(0, callback.afterEra, "a dropped source's callback was still called")

            // The id belongs to nobody now, so it can be taken again — by a source of its own with
            // a registration of its own.
            val reloaded = RecordingCallback()
            map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(reloaded))
            assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType(SOURCE))
            assertEquals(0, callback.afterEra, "the dropped callback was called for the new source")

            // A style by URL, served from the page so that nothing here waits on a network. It has
            // not replaced anything yet, so the registration is still the map's — which is what
            // makes the event below the moment it stops being.
            runtime.setResourceProvider { request, handle ->
              if (request.requestedUrl != STYLE_URL) {
                return@setResourceProvider ResourceProviderDecision.PASS_THROUGH
              }
              handle.complete(
                ResourceResponse(ResourceResponseStatus.OK).apply {
                  bytes = backgroundStyle(BACKGROUND_COLOR).encodeToByteArray()
                }
              )
              ResourceProviderDecision.PASS_THROUGH
            }
            map.setStyleUrl(STYLE_URL)
            reloaded.closeEra()
            assertEquals(
              registrationsBefore + 1,
              CustomGeometryBridge.liveRegistrations,
              "setting a style by URL released a registration before the style had loaded",
            )

            waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
            assertEquals(
              registrationsBefore,
              CustomGeometryBridge.liveRegistrations,
              "the loaded style left the dropped source's registration behind",
            )
            renderTurns(runtime, session, TEARDOWN_ATTEMPTS)
            assertEquals(0, reloaded.afterEra, "a dropped source's callback was still called")
          } finally {
            session.close()
          }
        } finally {
          context.close()
        }
      }
    }
  }

  /** Records what it was asked for, and whether anything arrived after its source was retired. */
  private class RecordingCallback : CustomGeometrySourceCallback {
    val tiles = mutableListOf<CanonicalTileId>()
    var afterEra = 0
      private set

    private var retired = false

    /** Marks the point past which this callback must never be called again. */
    fun closeEra() {
      retired = true
    }

    override fun fetchTile(tileId: CanonicalTileId) {
      if (retired) afterEra++ else tiles.add(tileId)
    }

    override fun cancelTile(tileId: CanonicalTileId) {
      if (retired) afterEra++
    }
  }

  /**
   * Renders until [predicate] holds, pumping the runtime in between.
   *
   * Both halves matter. Rendering is what makes MapLibre decide which tiles it needs, so a request
   * only reaches the page while frames are being drawn; and a pump parks this stack on the owner
   * thread, which hands the page back to its event loop — where the module's proxied notification
   * is waiting to be delivered.
   */
  private fun renderUntil(
    runtime: RuntimeHandle,
    session: RenderSessionHandle,
    predicate: () -> Boolean,
  ): Boolean {
    repeat(ATTEMPTS) {
      if (predicate()) return true
      session.renderUpdate()
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
    return predicate()
  }

  /**
   * Renders [turns] frames, giving the page a task between each.
   *
   * Used where the claim is that nothing arrives: a notification the module already posted is
   * delivered on a page task, so the page has to be handed back that many times before its absence
   * means anything.
   */
  private fun renderTurns(runtime: RuntimeHandle, session: RenderSessionHandle, turns: Int) {
    repeat(turns) {
      session.renderUpdate()
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
  }

  /** Pumps [turns] times, for after the map that was rendering has been closed. */
  private fun pumpTurns(runtime: RuntimeHandle, turns: Int) {
    repeat(turns) {
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
  }

  /**
   * Renders until the session has nothing left to draw.
   *
   * A render draws the parameters the map last handed the renderer, so the first one after a change
   * still paints what came before it; the frame that matters is the last one.
   */
  private fun renderUntilSettled(runtime: RuntimeHandle, session: RenderSessionHandle) {
    var rendered = false
    repeat(ATTEMPTS) {
      if (session.renderUpdate()) {
        rendered = true
      } else if (rendered) {
        return
      }
      runtime.pump(PUMP_MILLIS)
      while (runtime.pollEvent() != null) {}
    }
  }

  /** A fill layer over the custom source, which is what makes MapLibre ask for its tiles. */
  private fun fillLayer(): JsonValue =
    JsonValue.ObjectValue(
      listOf(
        JsonValue.Member("id", JsonValue.StringValue(FILL_LAYER)),
        JsonValue.Member("type", JsonValue.StringValue("fill")),
        JsonValue.Member("source", JsonValue.StringValue(SOURCE)),
        JsonValue.Member(
          "paint",
          JsonValue.ObjectValue(
            listOf(JsonValue.Member("fill-color", JsonValue.StringValue(FILL_COLOR)))
          ),
        ),
      )
    )

  /**
   * One polygon covering the world, which fills the viewport at the zoom the map opens at.
   *
   * A feature collection rather than a bare geometry, because MapLibre tiles a custom source's data
   * only when it is one: anything else leaves the tile with no features and the fill invisible.
   */
  private fun worldFill(): GeoJson =
    GeoJson.FeatureCollection(
      listOf(
        Feature(
          Geometry.Polygon(
            listOf(
              listOf(
                LatLng(-85.0, -180.0),
                LatLng(-85.0, 180.0),
                LatLng(85.0, 180.0),
                LatLng(85.0, -180.0),
                LatLng(-85.0, -180.0),
              )
            )
          ),
          emptyList(),
          FeatureIdentifier.Null,
        )
      )
    )

  private companion object {
    const val WIDTH = PageCanvases.WIDTH
    const val HEIGHT = PageCanvases.HEIGHT

    const val SOURCE = "custom-geometry"
    // Served by the provider above rather than fetched, so nothing here waits on a network.
    const val STYLE_URL = "https://example.invalid/custom-geometry-style.json"
    const val FILL_LAYER = "custom-geometry-fill"

    // Three distinct channels each, and neither colour is another test's, so a canvas showing
    // somebody else's frame fails rather than passing.
    const val BACKGROUND_COLOR = "#2060a0"
    const val BACKGROUND_RED = 0x20
    const val BACKGROUND_GREEN = 0x60
    const val BACKGROUND_BLUE = 0xA0

    const val FILL_COLOR = "#a06020"
    const val FILL_RED = 0xA0
    const val FILL_GREEN = 0x60
    const val FILL_BLUE = 0x20

    const val ATTEMPTS = 200
    const val TEARDOWN_ATTEMPTS = 32
    const val PUMP_MILLIS = 2L
  }
}
