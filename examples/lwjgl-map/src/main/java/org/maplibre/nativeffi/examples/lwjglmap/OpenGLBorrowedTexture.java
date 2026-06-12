package org.maplibre.nativeffi.examples.lwjglmap;

final class OpenGLBorrowedTexture implements AutoCloseable {
  private static final int TEXTURE_TARGET = org.lwjgl.opengl.GL11.GL_TEXTURE_2D;

  private final OpenGLContext context;
  private int texture;

  OpenGLBorrowedTexture(OpenGLContext context, Viewport viewport) {
    this.context = context;
    context.makeCurrent();
    texture =
        context.isGles()
            ? org.lwjgl.opengles.GLES20.glGenTextures()
            : org.lwjgl.opengl.GL11.glGenTextures();
    bindTexture(texture);
    texParameteri(org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER, org.lwjgl.opengl.GL11.GL_LINEAR);
    texParameteri(org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER, org.lwjgl.opengl.GL11.GL_LINEAR);
    texImage2D(viewport.framebufferWidth(), viewport.framebufferHeight());
    bindTexture(0);
    checkError("create OpenGL borrowed texture");
  }

  int texture() {
    return texture;
  }

  int target() {
    return TEXTURE_TARGET;
  }

  @Override
  public void close() {
    if (texture == 0) {
      return;
    }
    context.makeCurrent();
    if (context.isGles()) {
      org.lwjgl.opengles.GLES20.glDeleteTextures(texture);
    } else {
      org.lwjgl.opengl.GL11.glDeleteTextures(texture);
    }
    texture = 0;
  }

  private void bindTexture(int texture) {
    if (context.isGles()) {
      org.lwjgl.opengles.GLES20.glBindTexture(TEXTURE_TARGET, texture);
    } else {
      org.lwjgl.opengl.GL11.glBindTexture(TEXTURE_TARGET, texture);
    }
  }

  private void texParameteri(int pname, int value) {
    if (context.isGles()) {
      org.lwjgl.opengles.GLES20.glTexParameteri(TEXTURE_TARGET, pname, value);
    } else {
      org.lwjgl.opengl.GL11.glTexParameteri(TEXTURE_TARGET, pname, value);
    }
  }

  private void texImage2D(int width, int height) {
    if (context.isGles()) {
      org.lwjgl.opengles.GLES20.glTexImage2D(
          TEXTURE_TARGET,
          0,
          org.lwjgl.opengles.GLES30.GL_RGBA8,
          width,
          height,
          0,
          org.lwjgl.opengl.GL11.GL_RGBA,
          org.lwjgl.opengl.GL11.GL_UNSIGNED_BYTE,
          0L);
    } else {
      org.lwjgl.opengl.GL11.glTexImage2D(
          TEXTURE_TARGET,
          0,
          org.lwjgl.opengl.GL11.GL_RGBA8,
          width,
          height,
          0,
          org.lwjgl.opengl.GL11.GL_RGBA,
          org.lwjgl.opengl.GL11.GL_UNSIGNED_BYTE,
          0L);
    }
  }

  private void checkError(String operation) {
    var error =
        context.isGles()
            ? org.lwjgl.opengles.GLES20.glGetError()
            : org.lwjgl.opengl.GL11.glGetError();
    if (error != org.lwjgl.opengl.GL11.GL_NO_ERROR) {
      throw new IllegalStateException(
          "%s failed with OpenGL error 0x%x".formatted(operation, error));
    }
  }
}
