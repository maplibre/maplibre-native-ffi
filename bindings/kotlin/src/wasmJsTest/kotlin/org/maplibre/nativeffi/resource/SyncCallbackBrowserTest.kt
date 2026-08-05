package org.maplibre.nativeffi.resource

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/**
 * Registering and clearing the host callbacks MapLibre invokes from its own threads.
 *
 * Registration is the whole subject here, and specifically the moment a trampoline has to exist in
 * the linked binary: they are reached from JavaScript by name, and a name inside a JavaScript
 * string is invisible to dead-code elimination, so a binary that dropped one fails here at the
 * first `addFunction` rather than at the first callback. That is a real regression this locks, and
 * it is cheap to run because nothing has to load a style to reach it.
 *
 * **It does not cover the proxy.** Nothing here blocks a MapLibre worker inside
 * `emscripten_proxy_sync` or carries a decision back from the page, so a `sync_callback.c` that
 * proxied to the wrong queue, tripped the self-proxy assertion, or never wrote the decision back
 * would leave this file green. That path is covered by `ResourceProviderBrowserTest` and the
 * transform tests, which drive it end to end against a real style load; read those to know whether
 * the worker-to-page journey still works.
 */
class SyncCallbackBrowserTest {
  @Test
  fun aResourceProviderIsRegisteredAndCleared(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        // Registering is what places the trampoline in the module's function table and hands the
        // module's thunk the pointer that reaches it. A binary the export was eliminated from
        // fails here, because what reaches `addFunction` is undefined rather than a function.
        runtime.setResourceProvider { _, _ -> ResourceProviderDecision.PASS_THROUGH }

        // Replacing a live registration exercises the ordering the module's C header requires: the
        // new host is installed before the runtime is told about it, and the old one is only
        // cleared once the call that replaced it has returned.
        runtime.setResourceProvider { _, _ -> ResourceProviderDecision.PASS_THROUGH }

        runtime.clearResourceProvider()
        // Native accepts clearing a provider that was never set, so a host tearing down
        // unconditionally does not have to remember whether it registered one.
        runtime.clearResourceProvider()
      } finally {
        runtime.close()
      }
    }
  }

  @Test
  fun aResourceTransformIsRegisteredAndCleared(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        runtime.setResourceTransform { null }
        runtime.clearResourceTransform()
      } finally {
        runtime.close()
      }
    }
  }

  /**
   * The one host callback a browser cannot carry.
   *
   * The browser's fetch transport follows redirects itself, so there is no point at which an
   * outgoing header set could be handed back for rewriting. The binding specification documents
   * this as a permanent divergence rather than something left unimplemented, so it is asserted here
   * to keep it from being quietly "fixed" into something that does not work.
   */
  @Test
  fun anOutgoingHeaderTransformIsRefused(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      try {
        assertFailsWith<UnsupportedFeatureException> {
          runtime.setHttpHeaderTransform { emptyList() }
        }
        // Clearing one that could never be set still succeeds, so teardown stays unconditional.
        runtime.clearHttpHeaderTransform()
      } finally {
        runtime.close()
      }
    }
  }

  /** A provider survives the runtime it was registered on being closed with it still installed. */
  @Test
  fun aRuntimeClosesWithAProviderStillRegistered(): Promise<JsAny?> = browserTest {
    maplibreScope {
      val runtime = RuntimeHandle.create(RuntimeOptions())
      runtime.setResourceProvider { _, _ -> ResourceProviderDecision.PASS_THROUGH }
      // Closing without clearing is what a host that simply drops its runtime does. The trampoline
      // has to come out of the module's table anyway, or it outlives the callback it reaches.
      runtime.close()
      assertEquals(true, runtime.isClosed)
    }
  }
}
