package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.RenderSessionState
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
      sessions.enroll(session.binding)
      // A fresh state stands in for a session that its host destroys after abandonAll takes its
      // snapshot, so abandonAll passes the destroyed handle to native.
      sessions.enroll(HandleState("RenderSessionHandle", destroyed.binding.handle) {})
      sessions.abandonAll()
      assertEquals(RenderSessionState.ABANDONED, session.getSnapshot().state)
    }
  }

  @Test
  fun aSessionEnrolledAfterAbandonAllIsAbandonedAtOnce() = runSuspendTest {
    val sessions = ExitSessions()
    sessions.abandonAll()
    withOwnedTexture {
      sessions.enroll(session.binding)
      assertEquals(RenderSessionState.ABANDONED, session.getSnapshot().state)
    }
  }
}
