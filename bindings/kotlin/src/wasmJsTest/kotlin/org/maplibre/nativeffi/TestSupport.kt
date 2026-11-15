@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.delivery.HostDelivery

/**
 * Calls the binding's upcall sites as the module's trampolines do. The module exports no function
 * table, so a test can't call a function pointer that it gave native: it names the pointer's C
 * callback type, as the trampoline behind the pointer does.
 */
internal object TestSupport {
  fun callCompletion(callback: Long, userData: Long, result: Long) {
    check(callback == UpcallStubs.completion) { "$callback is not the completion trampoline" }
    HostDelivery.upcall("mln_completion_callback", arguments(userData.toInt(), result.toInt()))
  }

  fun callRelease(release: Long, userData: Long) {
    check(release == UpcallStubs.completionRelease) { "$release is not the release trampoline" }
    HostDelivery.upcall("mln_user_data_release", arguments(userData.toInt()))
  }
}

private fun arguments(first: Int): JsAny = js("[first >>> 0]")

private fun arguments(first: Int, second: Int): JsAny = js("[first >>> 0, second >>> 0]")
