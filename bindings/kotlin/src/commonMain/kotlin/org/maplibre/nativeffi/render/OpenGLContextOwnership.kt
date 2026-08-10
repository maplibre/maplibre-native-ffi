package org.maplibre.nativeffi.render

import kotlin.jvm.JvmInline

/**
 * How a render session's OpenGL context relates to the thread that attached it.
 *
 * A shared session leaves the thread as it found it: every render makes the session context current
 * and restores whatever was current before. The session context joins the share group named by the
 * context descriptor, so a host may hand the session a texture and sample it from its own context.
 *
 * A dedicated session owns the thread. It makes its context current once and keeps it current
 * between renders, and it joins no share group. Use this when a thread exists to drive one render
 * session and runs no other graphics work, such as an Android host that renders into a SurfaceView.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
@JvmInline
public value class OpenGLContextOwnership(public val nativeValue: Int) {
  public companion object {
    /** The session shares its thread with host graphics work. */
    public val SHARED: OpenGLContextOwnership = OpenGLContextOwnership(0)

    /** The session owns its thread's OpenGL context. */
    public val DEDICATED: OpenGLContextOwnership = OpenGLContextOwnership(1)
  }
}
