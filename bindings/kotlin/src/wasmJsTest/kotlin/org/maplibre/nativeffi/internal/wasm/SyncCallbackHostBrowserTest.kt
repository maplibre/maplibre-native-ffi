package org.maplibre.nativeffi.internal.wasm

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.browserTest

@JsFun("(host) => globalThis.__maplibreNativeC._mln_browser_sync_provider_install(host)")
private external fun installSyncProviderHost(host: Int): Boolean

@JsFun("(host) => globalThis.__maplibreNativeC._mln_browser_sync_transform_install(host)")
private external fun installSyncTransformHost(host: Int): Boolean

/**
 * The module's synchronous callback slots, driven directly rather than through a registration.
 *
 * Each slot is a module global, while the `user_data` that reaches it indexes a registry the
 * installing host keeps. A second host replacing the first would therefore not merely take over:
 * the first host's registrations are still registered with native, so its requests would arrive at
 * the second host carrying tokens that mean something else there. That is the failure this covers,
 * and it belongs to the module rather than to the binding, so it is reached through the entry
 * points a host calls. There is no C test for it because it exists only in the browser module.
 */
class SyncCallbackHostBrowserTest {
  @Test
  fun theModuleServesOneSynchronousCallbackHost(): Promise<JsAny?> = browserTest {
    assertSlotServesOneHost(::installSyncProviderHost, "resource provider")
    assertSlotServesOneHost(::installSyncTransformHost, "resource transform")
  }

  /**
   * Takes a slot, checks what a second host is told, and gives the slot back.
   *
   * The first install is what makes the rest of this safe as well as what starts it. Installing
   * succeeds only from an empty slot, so a slot this suite has left occupied fails here rather than
   * being cleared out from under whatever occupied it -- and an empty slot is one nothing has
   * registered a thunk against, so the pointers below are never called.
   */
  private fun assertSlotServesOneHost(install: (Int) -> Boolean, subject: String) {
    assertTrue(install(FIRST_HOST), "the $subject slot was already taken when this test began")
    assertTrue(install(FIRST_HOST), "the $subject host was refused a reinstall of itself")
    assertFalse(install(SECOND_HOST), "a second $subject host was allowed to replace the first")
    assertTrue(install(NO_HOST), "clearing the $subject host was refused")
    assertTrue(install(SECOND_HOST), "the $subject slot was still taken after it was cleared")
    assertTrue(install(NO_HOST), "clearing the second $subject host was refused")
  }

  private companion object {
    /** The null pointer the module reads as a clear. */
    const val NO_HOST = 0

    /**
     * Two host pointers that stand for two hosts and are never called.
     *
     * A host pointer is a function-table index, and these name whatever happens to sit at those
     * indices. That is harmless here because the module reaches one only through a thunk a runtime
     * registered, and the first assertion above establishes that nothing has.
     */
    const val FIRST_HOST = 1
    const val SECOND_HOST = 2
  }
}
