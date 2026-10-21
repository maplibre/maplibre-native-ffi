package org.maplibre.nativeffi.internal.loader

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest

/** The one module load that every `Maplibre.loadModule` caller shares. */
class SharedLoadTest {
  @Test
  fun aCancelledCallerLeavesTheLoadToTheOthers() = runSuspendTest {
    val shared = SharedLoad()
    val started = CompletableDeferred<Unit>()
    val finish = CompletableDeferred<Unit>()
    var loads = 0
    val load: suspend () -> Unit = {
      loads += 1
      started.complete(Unit)
      finish.await()
    }
    val first = async(start = CoroutineStart.UNDISPATCHED) { shared.await(load) }
    val second = async(start = CoroutineStart.UNDISPATCHED) { shared.await(load) }
    started.awaitWithin("the load to start")
    first.cancel()
    first.join()
    finish.complete(Unit)

    second.awaitWithin("the second caller's load")
    assertTrue(first.isCancelled)
    shared.await(load)
    assertEquals(1, loads, "the shared load ran more than once")
  }

  @Test
  fun aFailedLoadLetsALaterCallLoadAgain() = runSuspendTest {
    val shared = SharedLoad()
    assertFailsWith<IllegalStateException> { shared.await { error("the module failed to load") } }
    var loaded = false
    shared.await { loaded = true }
    assertTrue(loaded, "the later call did not load again")
  }
}
