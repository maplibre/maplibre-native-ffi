package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.opengl.GL11.GL_LINEAR;
import static org.lwjgl.opengl.GL11.GL_RGBA;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_2D;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER;
import static org.lwjgl.opengl.GL11.GL_UNSIGNED_BYTE;
import static org.lwjgl.opengl.GL11.glBindTexture;
import static org.lwjgl.opengl.GL11.glDeleteTextures;
import static org.lwjgl.opengl.GL11.glGenTextures;
import static org.lwjgl.opengl.GL11.glTexImage2D;
import static org.lwjgl.opengl.GL11.glTexParameteri;
import static org.lwjgl.system.MemoryUtil.NULL;

final class OpenGLBorrowedTexture implements AutoCloseable {
  private final OpenGLContext context;
  private int texture;

  OpenGLBorrowedTexture(OpenGLContext context, Viewport viewport) {
    this.context = context;
    context.makeCurrent();
    texture = glGenTextures();
    if (texture == 0) {
      throw new IllegalStateException("glGenTextures returned 0");
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        viewport.framebufferWidth(),
        viewport.framebufferHeight(),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        NULL);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  int texture() {
    return texture;
  }

  int target() {
    return GL_TEXTURE_2D;
  }

  @Override
  public void close() {
    if (texture == 0) {
      return;
    }
    context.makeCurrent();
    glDeleteTextures(texture);
    texture = 0;
  }
}
