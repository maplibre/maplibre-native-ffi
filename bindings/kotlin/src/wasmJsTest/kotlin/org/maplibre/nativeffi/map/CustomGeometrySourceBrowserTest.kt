package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertContains
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.PageCanvas
import org.maplibre.nativeffi.assertPresentedColor
import org.maplibre.nativeffi.backgroundStyle
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.wasm.CustomGeometryBridge
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.pumpTurns
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.render.WebglContext
import org.maplibre.nativeffi.resource.ResourceProviderRoute
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.waitForMapEvent
import org.maplibre.nativeffi.withMap

/**
 * A custom geometry source whose tiles this binding supplies, end to end.
 *
 * MapLibre asks for a tile from the worker the source's tile loader runs on, and that worker cannot
 * enter this WebAssembly instance. So the C shim copies the tile id into the module's record ring
 * and the binding delivers it while draining that ring inside `pump`. The body therefore runs on an
 * ordinary stack, after the pump's own C call has returned, and may answer with
 * `setCustomGeometrySourceTileData` from inside itself — which is what shared expect/actual code
 * written for JVM, Android, or Kotlin/Native does — or record the tile and answer afterwards.
 *
 * Retirement travels in the same ring, behind the notifications it retires: the shim invokes the
 * tile callbacks once more with a tile id no real tile uses. So a source that is gone stops being
 * delivered when that marker comes out of the ring, and the notifications already queued for it
 * reach nobody.
 *
 * The proof of the working path has to be the whole chain rather than any part of it, and it has to
 * be both answers. A fill layer over the custom source paints one colour over a background of
 * another, so the canvas changing colour is the source's geometry arriving and nothing else.
 */
class CustomGeometrySourceBrowserTest {
  // Spec coverage: BND-124.
  @Test
  fun presentsTheTilesItsCallbackWasAskedForAndTheHostSupplied() {
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
      // captured rather than thrown, because nothing above a callback body would catch one and the
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

      val context = PageCanvas.context()
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

        // Nothing asks a custom source for a tile until a layer that uses it is being drawn, so the
        // request arrives while the map renders rather than when the source is added.
        assertTrue(
          renderUntil(runtime, session) { requested.isNotEmpty() },
          "the source never asked for a tile",
        )
        // The whole world at the zoom the map opens at, which is the tile the fill below covers.
        assertContains(requested, CanonicalTileId(0, 0, 0))

        // The layer has a source and the source has no data, so what the canvas holds so far is the
        // background alone. Asserted before the answer, so the colour below is provably the
        // geometry arriving rather than a frame that was already there.
        renderUntilSettled(runtime, session)
        assertPresentedColor(context, BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE)

        // The answer, from outside the callback. One polygon covering the world fills the viewport
        // at this zoom, so the fill colour is what the canvas must come to show.
        for (tileId in requested.toList()) {
          map.setCustomGeometrySourceTileData(SOURCE, tileId, worldFill())
        }
        assertTrue(
          renderUntil(runtime, session) { map.isFullyLoaded },
          "the map never finished loading the tiles the host supplied",
        )
        renderUntilSettled(runtime, session)
        assertPresentedColor(context, FILL_RED, FILL_GREEN, FILL_BLUE)

        // A cancel is best-effort and may never arrive, so nothing here waits for one. What is true
        // whenever one does arrive is that it names a tile this source asked for.
        for (tileId in cancelled) assertContains(requested, tileId)

        // Removing the layer and then the source takes the geometry away again, which is the other
        // half of the claim: the fill was the source's rather than anything the style held on its
        // own. It also puts the canvas back to one colour, so the second half of this test starts
        // from a frame that provably holds no custom geometry.
        assertTrue(map.removeStyleLayer(FILL_LAYER))
        assertTrue(map.removeStyleSource(SOURCE))
        assertFalse(map.styleSourceExists(SOURCE))
        renderUntilSettled(runtime, session)
        assertPresentedColor(context, BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE)

        // The same source again, answered from inside the callback this time. This is the workflow
        // a multiplatform host writes once and runs everywhere, and the claim is that it is not
        // merely accepted here but that its geometry reaches the target: nothing between this and
        // the assertion below supplies a tile.
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
        assertPresentedColor(context, FILL_RED, FILL_GREEN, FILL_BLUE)
      } finally {
        session.close()
      }
    }
  }

  /**
   * The teardown paths, with tile requests in flight.
   *
   * A notification is a copy of a tile id sitting in the ring, so it outlives the source it belongs
   * to and can still be there when that source is gone. What the binding promises is that such a
   * notification reaches nobody — the retirement marker travels behind it, and delivery stops when
   * the marker comes out — and that a source added again under the same id is a new registration
   * rather than the old one coming back.
   *
   * Rendered into a texture of its own, because nothing here is about what a frame looks like and
   * the one page canvas belongs to whichever test is presenting.
   */
  // Spec coverage: BND-124.
  @Test
  fun dropsNotificationsForSourcesThatAreGoneAndReusesTheIdCleanly() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      val first = RecordingCallback()
      val second = RecordingCallback()
      // What the registry held before this test, so each teardown below can be asserted to have put
      // it back rather than to have reached zero by luck. A registration that outlives its source
      // is
      // invisible from the outside — it does nothing until native calls it, and the teardown is
      // what
      // made native calling it impossible — so this is what says it went.
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
            "the source never asked for a tile",
          )

          // Removed with the request still unanswered, so the tile is one MapLibre is still waiting
          // on: dropping the layer retires that tile and produces the cancels this source's loader
          // pushes into the ring, and the source goes before the ring has been drained. The layer
          // goes first because native refuses to remove a source a layer still uses. From here the
          // callback must hear nothing at all.
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

          // The same id again, and a different callback. A registration is reached through state
          // native carries back, and that state is fresh, so the source added here is a new
          // registration rather than the previous one under a familiar name.
          map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(second))
          map.addStyleLayerJson(fillLayer(), "")
          assertTrue(
            renderUntil(runtime, session) { second.tiles.isNotEmpty() },
            "the source added under the reused id never asked for a tile",
          )
          assertEquals(0, first.afterEra, "the replaced callback was called for the new source")

          // Closing the map with a source still registered is the last teardown path, and the one
          // that leaves native with no way to ask again: the map took its style, its sources, and
          // their tile loaders with it. Retiring those tiles makes their loader push cancels from
          // its own thread, which can land in the ring after the close has returned.
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

  /**
   * A style reload, which drops every source the previous style held.
   *
   * The registration behind a custom geometry source belongs to that source, so a style that
   * replaces it ends the registration too — while the tiles the previous style had are being
   * retired, which is exactly when their loader pushes the cancels this must not deliver.
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
  fun releasesSourcesAStyleReloadDropped() {
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
            "the source never asked for a tile",
          )

          map.setStyleJson(backgroundStyle(FILL_COLOR))
          callback.closeEra()
          // Asserted before anything is polled or rendered, because this is the claim about a style
          // set as JSON: the source is gone by the time the call returns, so its registration is
          // too.
          assertEquals(
            registrationsBefore,
            CustomGeometryBridge.liveRegistrations,
            "setting a style as JSON left the dropped source's registration behind",
          )
          assertFalse(map.styleSourceExists(SOURCE))
          renderTurns(runtime, session, TEARDOWN_ATTEMPTS)
          assertEquals(0, callback.afterEra, "a dropped source's callback was still called")

          // The id belongs to nobody now, so it can be taken again — by a source of its own with a
          // registration of its own.
          val reloaded = RecordingCallback()
          map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(reloaded))
          assertEquals(SourceType.CUSTOM_VECTOR, map.styleSourceType(SOURCE))
          assertEquals(0, callback.afterEra, "the dropped callback was called for the new source")

          // A style by URL, answered by a provider route so that nothing here waits on a network.
          // It
          // has not replaced anything yet, so the registration is still the map's — which is what
          // makes the event below the moment it stops being.
          runtime.setResourceProvider(listOf(ResourceProviderRoute(url = STYLE_URL))) { _, handle ->
            handle.complete(
              ResourceResponse(ResourceResponseStatus.OK).apply {
                bytes = backgroundStyle(BACKGROUND_COLOR).encodeToByteArray()
              }
            )
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

  /**
   * A map closed from inside its own tile callback, and the runtime that has to survive it.
   *
   * Closing a map releases the source registrations it holds, and releasing one waits for a
   * callback body that is already inside it. Here that body is the frame below the close, on the
   * one stack this target has, so the wait can never finish and the binding refuses it rather than
   * spinning forever.
   *
   * The refusal is not the claim. By the time it happens the map is destroyed and the wrapper is
   * closed, so closing again does nothing and no later call can finish what the teardown did not —
   * which makes the state this leaves behind permanent. The claim is therefore that the rest of the
   * accounting happened anyway: **the runtime is still closable**. If it were not, a host would be
   * left holding a runtime it can never close and a map that no longer exists, for the whole life
   * of the page, with nothing it could do about either.
   */
  @Test
  fun aMapWhoseSourceTeardownFailedStillGivesUpItsRuntime() {
    var closeFailure: Throwable? = null
    var closed = false
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = WIDTH
          height = HEIGHT
        },
      )
    val callback =
      object : CustomGeometrySourceCallback {
        override fun fetchTile(tileId: CanonicalTileId) {
          // Only the first tile closes. The rest return at once, so exactly one body is inside the
          // registration when the teardown begins.
          if (closed) return
          closed = true
          closeFailure = runCatching { map.close() }.exceptionOrNull()
        }

        override fun cancelTile(tileId: CanonicalTileId) {}
      }

    val registrationsBefore = CustomGeometryBridge.liveRegistrations
    val context = WebglContext.createOffscreen(WIDTH, HEIGHT)
    val session =
      map.attachOpenGLOwnedTexture(
        OpenGLOwnedTextureDescriptor(RenderTargetExtent(WIDTH, HEIGHT, 1.0), context.descriptor())
      )
    map.setStyleJson(backgroundStyle(BACKGROUND_COLOR))
    waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED)
    map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(callback))
    map.addStyleLayerJson(fillLayer(), "")
    assertTrue(
      renderUntil(runtime, session) { closed },
      "no tile callback ever ran, so the close below never happened",
    )
    session.close()
    context.close()

    val failure = assertIs<InvalidStateException>(closeFailure, "closing reported $closeFailure")
    assertContains(failure.diagnostic, "callback")
    // The map is gone whatever the teardown did: native destroyed it, and a wrapper that called
    // itself live afterwards would offer calls that could only fail.
    assertTrue(map.isClosed, "a map whose teardown failed was left claiming to be open")
    assertEquals(
      registrationsBefore,
      CustomGeometryBridge.liveRegistrations,
      "a source whose release was refused kept its registration, so a late tile could still reach it",
    )

    // The claim. A runtime the closed map still retained would refuse this for the life of the
    // page,
    // and nothing could release it.
    val runtimeFailure = runCatching { runtime.close() }.exceptionOrNull()
    assertNull(
      runtimeFailure,
      "the runtime could not be closed after its map's source teardown failed: $runtimeFailure",
    )
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
   * A source native refuses leaves the callback already registered under that id serving tiles.
   *
   * This family needs nothing injected. A style holds one source per id, so adding a second under
   * an id it already carries is refused by native itself — which is precisely a replacement
   * failing, and it fails at the point that matters: the binding has already installed the
   * replacement's registration state, because the shim reaches a tile callback through the pointer
   * that installation places and a source added first could ask for a tile with nowhere to send it.
   *
   * So the refusal has to give that state back and leave the previous one alone, and both halves
   * are asserted: the registry count says the replacement's state went, and the tiles that are
   * still asked for say whose callback native reaches.
   */
  // Spec coverage: BND-122.
  @Test
  fun aSourceReplacementNativeRefusesKeepsThePreviousCallback() {
    withMap(WIDTH, HEIGHT) { runtime, map ->
      val installed = RecordingCallback()
      val refused = RecordingCallback()
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
          map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(installed))
          map.addStyleLayerJson(fillLayer(), "")
          assertTrue(
            renderUntil(runtime, session) { installed.tiles.isNotEmpty() },
            "the source never asked for a tile",
          )
          assertEquals(registrationsBefore + 1, CustomGeometryBridge.liveRegistrations)

          // The same id again. Native holds one source per id and says so.
          val error =
            assertFailsWith<InvalidArgumentException> {
              map.addCustomGeometrySource(SOURCE, CustomGeometrySourceOptions(refused))
            }
          assertContains(error.diagnostic, "already exists")

          // The refused source's registration went back rather than holding the shim's listener
          // open for a source that does not exist.
          assertEquals(
            registrationsBefore + 1,
            CustomGeometryBridge.liveRegistrations,
            "the refused source left its registration behind",
          )

          // And native still asks the callback that was already there, which is the half the count
          // cannot show.
          val askedBefore = installed.tiles.size
          map.invalidateCustomGeometrySourceTile(SOURCE, installed.tiles.first())
          assertTrue(
            renderUntil(runtime, session) { installed.tiles.size > askedBefore },
            "the source stopped asking the callback it was added with",
          )
          assertTrue(refused.tiles.isEmpty(), "the refused source's callback was called anyway")

          assertTrue(map.removeStyleLayer(FILL_LAYER))
          assertTrue(map.removeStyleSource(SOURCE))
          assertEquals(registrationsBefore, CustomGeometryBridge.liveRegistrations)
        } finally {
          session.close()
        }
      } finally {
        context.close()
      }
    }
  }

  /**
   * Renders until [predicate] holds, pumping the runtime in between.
   *
   * Both halves matter. Rendering is what makes MapLibre decide which tiles it needs, so a request
   * is only produced while frames are being drawn; and the pump is what drains the ring the request
   * arrived in.
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

  /** Renders [turns] frames, draining the ring between each. */
  private fun renderTurns(runtime: RuntimeHandle, session: RenderSessionHandle, turns: Int) {
    repeat(turns) {
      session.renderUpdate()
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
    const val WIDTH = PageCanvas.WIDTH
    const val HEIGHT = PageCanvas.HEIGHT

    const val SOURCE = "custom-geometry"
    // Answered by the provider above rather than fetched, so nothing here waits on a network.
    const val STYLE_URL = "custom://custom-geometry-style.json"
    const val FILL_LAYER = "custom-geometry-fill"

    // Three distinct channels each, so a path that swapped or duplicated one would still be caught.
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
