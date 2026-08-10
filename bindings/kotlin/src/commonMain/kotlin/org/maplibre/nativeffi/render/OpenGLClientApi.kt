package org.maplibre.nativeffi.render

import kotlin.jvm.JvmInline

/**
 * OpenGL client API a dedicated EGL render session creates its context for.
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch. Unknown values keep their raw [nativeValue].
 */
@JvmInline
public value class OpenGLClientApi(public val nativeValue: Int) {
  public companion object {
    /** No client API is named. */
    public val UNSPECIFIED: OpenGLClientApi = OpenGLClientApi(0)

    /** Desktop OpenGL, as `EGL_OPENGL_API` names it. */
    public val GL: OpenGLClientApi = OpenGLClientApi(1)

    /** OpenGL ES, as `EGL_OPENGL_ES_API` names it. */
    public val GLES: OpenGLClientApi = OpenGLClientApi(2)
  }
}
