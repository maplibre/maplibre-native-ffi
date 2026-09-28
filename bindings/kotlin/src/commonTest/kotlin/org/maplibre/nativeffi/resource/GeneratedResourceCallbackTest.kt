package org.maplibre.nativeffi.resource

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.withTimeout
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision as Decision
import org.maplibre.nativeffi.generated.ResourceResponse as Response
import org.maplibre.nativeffi.generated.ResourceResponseStatus as ResponseStatus
import org.maplibre.nativeffi.runtime.runSuspendTest

class GeneratedResourceCallbackTest {
  @Test
  fun malformedResponseCanBeRetriedAndInlineCloseClaimsRequest(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val completed = CompletableDeferred<Unit>()
    try {
      runtime
        .setResourceProvider(
          ResourceProvider { _, request ->
            try {
              assertFailsWith<InvalidArgumentException> {
                request.resourceRequestComplete(
                  Response(ResponseStatus.ERROR, errorMessage = "bad\u0000message")
                )
              }
              assertFailsWith<InvalidStateException> { runtime.barrier() }
              request.resourceRequestComplete(
                Response(
                  ResponseStatus.OK,
                  bytes = """{"version":8,"sources":{},"layers":[]}""".encodeToByteArray(),
                )
              )
              request.close()
              request.close()
              completed.complete(Unit)
            } catch (error: Throwable) {
              completed.completeExceptionally(error)
            }
            Decision.PASS_THROUGH
          }
        )
        .await()
      val map = runtime.mapCreate(GeneratedApi.mapOptionsDefault()).await()
      try {
        map.setStyleUrl("binding-test://response").await()
        withTimeout(5_000) { completed.await() }
        runtime.barrier().await()
      } finally {
        map.close().await()
      }
    } finally {
      runtime.close().await()
    }
  }
}
