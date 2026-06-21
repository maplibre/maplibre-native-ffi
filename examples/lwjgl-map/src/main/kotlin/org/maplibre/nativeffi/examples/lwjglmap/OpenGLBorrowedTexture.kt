package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.opengl.GL11
import org.lwjgl.opengles.GLES20
import org.lwjgl.opengles.GLES30

internal class OpenGLBorrowedTexture(graphicsContext: GraphicsContext, viewport: Viewport) :
  AutoCloseable {
  private val openGLContext = graphicsContext as OpenGLContext
  private var texture: Int

  init {
    openGLContext.makeCurrent()
    texture =
      if (openGLContext.isGles) {
        GLES20.glGenTextures()
      } else {
        GL11.glGenTextures()
      }
    bindTexture(texture)
    texParameteri(GL11.GL_TEXTURE_MIN_FILTER, GL11.GL_LINEAR)
    texParameteri(GL11.GL_TEXTURE_MAG_FILTER, GL11.GL_LINEAR)
    texImage2D(viewport.framebufferWidth(), viewport.framebufferHeight())
    bindTexture(0)
    checkError("create OpenGL borrowed texture")
  }

  fun texture(): Int = texture

  fun target(): Int = TEXTURE_TARGET

  override fun close() {
    if (texture == 0) {
      return
    }
    openGLContext.makeCurrent()
    if (openGLContext.isGles) {
      GLES20.glDeleteTextures(texture)
    } else {
      GL11.glDeleteTextures(texture)
    }
    texture = 0
  }

  private fun bindTexture(texture: Int) {
    if (openGLContext.isGles) {
      GLES20.glBindTexture(TEXTURE_TARGET, texture)
    } else {
      GL11.glBindTexture(TEXTURE_TARGET, texture)
    }
  }

  private fun texParameteri(pname: Int, value: Int) {
    if (openGLContext.isGles) {
      GLES20.glTexParameteri(TEXTURE_TARGET, pname, value)
    } else {
      GL11.glTexParameteri(TEXTURE_TARGET, pname, value)
    }
  }

  private fun texImage2D(width: Int, height: Int) {
    if (openGLContext.isGles) {
      GLES20.glTexImage2D(
        TEXTURE_TARGET,
        0,
        GLES30.GL_RGBA8,
        width,
        height,
        0,
        GL11.GL_RGBA,
        GL11.GL_UNSIGNED_BYTE,
        0L,
      )
    } else {
      GL11.glTexImage2D(
        TEXTURE_TARGET,
        0,
        GL11.GL_RGBA8,
        width,
        height,
        0,
        GL11.GL_RGBA,
        GL11.GL_UNSIGNED_BYTE,
        0L,
      )
    }
  }

  private fun checkError(operation: String) {
    val error =
      if (openGLContext.isGles) {
        GLES20.glGetError()
      } else {
        GL11.glGetError()
      }
    if (error != GL11.GL_NO_ERROR) {
      throw IllegalStateException("%s failed with OpenGL error 0x%x".format(operation, error))
    }
  }

  private companion object {
    private const val TEXTURE_TARGET = GL11.GL_TEXTURE_2D
  }
}
