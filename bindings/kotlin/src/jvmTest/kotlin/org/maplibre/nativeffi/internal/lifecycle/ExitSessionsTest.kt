package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.generated.RenderSessionHandle
import org.maplibre.nativeffi.generated.RenderSessionState
import org.maplibre.nativeffi.render.withOwnedTexture
import org.maplibre.nativeffi.runSuspendTest

/** The render sessions that the JVM shutdown hook abandons. */
class ExitSessionsTest {
  @Test
  fun abandonAllAbandonsALiveSessionAndPassesOverAReleasedOne() = runSuspendTest {
    val sessions = ExitSessions()
    val destroyed = withOwnedTexture {
      complete(session.detach(), "the detach")
      session.close()
      session
    }
    withOwnedTexture {
      sessions.enroll(session)
      // A fresh report stands in for a session that its host destroys after abandonAll takes its
      // snapshot, so abandonAll passes the destroyed handle to native.
      val handle = destroyed.binding.issued()
      sessions.enroll(handle, HandleStateCore.LeakReport("RenderSessionHandle", handle))
      sessions.abandonAll()
      assertEquals(RenderSessionState.ABANDONED, session.getSnapshot().state)
    }
  }

  @Test
  fun aSessionEnrolledAfterAbandonAllIsAbandonedAtOnce() = runSuspendTest {
    val sessions = ExitSessions()
    sessions.abandonAll()
    withOwnedTexture {
      sessions.enroll(session)
      assertEquals(RenderSessionState.ABANDONED, session.getSnapshot().state)
    }
  }

  private fun ExitSessions.enroll(session: RenderSessionHandle) =
    enroll(session.binding.issued(), session.binding.leakReport)
}
