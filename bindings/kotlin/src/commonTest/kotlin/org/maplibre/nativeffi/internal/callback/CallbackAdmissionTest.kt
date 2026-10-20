package org.maplibre.nativeffi.internal.callback

import kotlin.test.Test
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidStateException

/** The per-thread callback scopes that decide which native calls a callback may make. */
class CallbackAdmissionTest {
  @Test
  fun aScopeAdmitsOnlyItsOwnersAllowedCallsAndNestedScopesNarrowFurther() {
    CallbackAdmission.scope(owner = 1L, allowed = setOf("complete", "release")).use {
      CallbackAdmission.check(1L, "complete")
      assertFailsWith<InvalidStateException> { CallbackAdmission.check(2L, "complete") }
      assertFailsWith<InvalidStateException> { CallbackAdmission.check(1L, "resize") }

      CallbackAdmission.scope(owner = 1L, allowed = setOf("release")).use {
        CallbackAdmission.check(1L, "release")
        // The outer scope's allowance does not widen the inner one.
        assertFailsWith<InvalidStateException> { CallbackAdmission.check(1L, "complete") }
      }
      CallbackAdmission.check(1L, "complete")
    }
    // Outside every scope, any call is admitted.
    CallbackAdmission.check(2L, "resize")
  }
}
