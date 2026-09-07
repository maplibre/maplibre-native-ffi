package org.maplibre.nativeffi.runtime

import kotlin.test.assertEquals
import kotlin.test.assertTrue
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.error.MaplibreStatus

/** Awaits one ordered command and asserts that it committed. */
internal suspend fun Deferred<CommandCompletion>.awaitCommitted(): CommandCompletion {
  val completion = await()
  assertCommitted(completion)
  return completion
}

/** Awaits one ordered command and asserts that its body rejected the work with [status]. */
internal suspend fun Deferred<CommandCompletion>.awaitFailed(
  status: MaplibreStatus
): CommandCompletion {
  val completion = await()
  assertEquals(CommandDisposition.FAILED, completion.disposition)
  assertEquals(status, completion.status)
  assertTrue(completion.diagnostic.isNotEmpty(), "a failed command carries a diagnostic")
  return completion
}

/** Asserts that an already-awaited command committed. */
internal fun assertCommitted(completion: CommandCompletion) {
  assertEquals(CommandDisposition.COMMITTED, completion.disposition)
  assertEquals(MaplibreStatus.OK, completion.status)
}
