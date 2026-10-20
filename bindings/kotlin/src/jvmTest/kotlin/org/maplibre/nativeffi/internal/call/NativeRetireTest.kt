package org.maplibre.nativeffi.internal.call

import java.lang.reflect.Modifier
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import org.maplibre.nativeffi.generated.CameraTransitionEnd
import org.maplibre.nativeffi.generated.CameraTransitionHandler
import org.maplibre.nativeffi.generated.CameraTransitionOutcome
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.internal.async.callCompletion
import org.maplibre.nativeffi.internal.async.callCompletionRelease
import org.maplibre.nativeffi.internal.c.CompletionLayout
import org.maplibre.nativeffi.internal.c.CompletionResultLayout
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope
import org.maplibre.nativeffi.internal.callback.CallbackRoots
import org.maplibre.nativeffi.internal.callback.NativeRoots
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readAddress
import org.maplibre.nativeffi.internal.memory.writeI32
import org.maplibre.nativeffi.internal.memory.writeU32

class NativeRetireTest {
  @Test
  fun retirementKeepsTerminalCallbacksUntilNativeReleasesTheBridge() {
    val delivered = mutableListOf<CameraTransitionEnd>()
    val (callback, completion) = retire { delivered.add(it) }
    val (complete, token, release) = completion
    try {
      assertRetainedOwner(token)
      val terminal = assertNotNull(CallbackRoots.get(callback)?.value) as CameraTransitionHandler
      terminal.callback!!(CameraTransitionEnd(outcome = CameraTransitionOutcome.CLOSED))
      assertEquals(listOf(CameraTransitionEnd(outcome = CameraTransitionOutcome.CLOSED)), delivered)
      CallbackRoots.release(callback)

      NativeArena().use { arena ->
        val result = arena.allocate(CompletionResultLayout.SIZEOF)
        writeU32(result + CompletionResultLayout.SIZE, CompletionResultLayout.SIZEOF.toUInt())
        writeI32(result + CompletionResultLayout.STATUS, Status.OK.rawValue)
        callCompletion(complete, token, result)
      }
      assertRetainedOwner(token)
    } finally {
      CallbackRoots.release(callback)
      callCompletionRelease(release, token)
    }
    assertNull(NativeRoots.get(token))
  }

  private class RetiringOwner {
    val callbacks = CallbackOwner()
  }

  // Only the native tokens escape this function; the caller keeps neither the owner nor its
  // deferred.
  private fun retire(deliver: (CameraTransitionEnd) -> Unit): Pair<Long, Triple<Long, Long, Long>> {
    val owner = RetiringOwner()
    val callback =
      CallbackRegistrationScope().use { registrations ->
        registrations.register(CameraTransitionHandler(deliver)).also {
          registrations.accept(owner.callbacks)
        }
      }
    var completion: Triple<Long, Long, Long>? = null
    nativeRetire(owner, HandleState("MapHandle", 1L) {}) {
      completion =
        Triple(
          readAddress(this.completion + CompletionLayout.CALLBACK),
          readAddress(this.completion + CompletionLayout.USER_DATA),
          readAddress(this.completion + CompletionLayout.RELEASE_USER_DATA),
        )
    }
    return callback to requireNotNull(completion)
  }

  // Inspect the strong bridge reference directly, independent of JVM collection scheduling.
  private fun assertRetainedOwner(token: Long) {
    val bridge = assertNotNull(NativeRoots.get(token))
    val convert =
      bridge.javaClass.getDeclaredField("convert").apply { isAccessible = true }.get(bridge)
    val owner =
      convert.javaClass.declaredFields
        .filter { !Modifier.isStatic(it.modifiers) }
        .map { it.apply { isAccessible = true }.get(convert) }
        .filterIsInstance<RetiringOwner>()
        .singleOrNull()
    assertNotNull(owner, "the native completion must strongly retain the retiring owner")
  }
}
