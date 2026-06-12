package org.maplibre.nativeffi.examples.lwjglmap;

import org.maplibre.nativeffi.render.OpenGLOwnedTextureFrameHandle;

final class OpenGLTextureCompositor implements AutoCloseable {
  private static final int TEXTURE_TARGET = org.lwjgl.opengl.GL11.GL_TEXTURE_2D;

  private final OpenGLContext context;
  private final boolean gles;
  private int program;
  private int vertexArray;
  private Viewport viewport;

  OpenGLTextureCompositor(OpenGLContext context, Viewport viewport) {
    this.context = context;
    this.gles = context.isGles();
    this.viewport = viewport;
    context.makeCurrent();
    try {
      program = createTextureProgram();
      vertexArray = genVertexArray();
      useProgram(program);
      var sampler = getUniformLocation(program, "map_texture");
      if (sampler >= 0) {
        uniform1i(sampler, 0);
      }
      useProgram(0);
      checkError("initialize OpenGL texture compositor");
    } catch (RuntimeException error) {
      close();
      throw error;
    }
  }

  void resize(Viewport viewport) {
    this.viewport = viewport;
  }

  void draw(OpenGLOwnedTextureFrameHandle frameHandle) {
    var frame = frameHandle.frame();
    if (frame.width() <= 0 || frame.height() <= 0) {
      throw new IllegalStateException("MapLibre returned an empty OpenGL owned texture frame");
    }
    if (frame.target() != TEXTURE_TARGET) {
      throw new IllegalStateException(
          "MapLibre owned texture target is %d, expected GL_TEXTURE_2D".formatted(frame.target()));
    }
    drawTexture(frame.texture());
  }

  void drawTexture(int texture) {
    if (texture == 0) {
      throw new IllegalStateException("OpenGL texture name is zero");
    }
    context.makeCurrent();
    bindFramebuffer(0);
    disable(org.lwjgl.opengl.GL11.GL_CULL_FACE);
    disable(org.lwjgl.opengl.GL11.GL_DEPTH_TEST);
    disable(org.lwjgl.opengl.GL11.GL_SCISSOR_TEST);
    viewport(0, 0, viewport.framebufferWidth(), viewport.framebufferHeight());
    clearColor(0.08f, 0.09f, 0.11f, 1.0f);
    clear(org.lwjgl.opengl.GL11.GL_COLOR_BUFFER_BIT);
    useProgram(program);
    bindVertexArray(vertexArray);
    activeTexture(org.lwjgl.opengl.GL13.GL_TEXTURE0);
    bindTexture(TEXTURE_TARGET, texture);
    texParameteri(
        TEXTURE_TARGET,
        org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER,
        org.lwjgl.opengl.GL11.GL_LINEAR);
    texParameteri(
        TEXTURE_TARGET,
        org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER,
        org.lwjgl.opengl.GL11.GL_LINEAR);
    drawArrays(org.lwjgl.opengl.GL11.GL_TRIANGLES, 0, 3);
    bindTexture(TEXTURE_TARGET, 0);
    bindVertexArray(0);
    useProgram(0);
    checkError("draw OpenGL texture");
    context.swapBuffers();
  }

  @Override
  public void close() {
    if (context != null) {
      try {
        context.makeCurrent();
      } catch (RuntimeException ignored) {
        return;
      }
    }
    if (vertexArray != 0) {
      deleteVertexArray(vertexArray);
      vertexArray = 0;
    }
    if (program != 0) {
      deleteProgram(program);
      program = 0;
    }
  }

  private int createTextureProgram() {
    var vertex =
        compileShader(org.lwjgl.opengl.GL20.GL_VERTEX_SHADER, vertexShaderSource(), "vertex");
    var fragment = 0;
    var linkedProgram = 0;
    try {
      fragment =
          compileShader(
              org.lwjgl.opengl.GL20.GL_FRAGMENT_SHADER, fragmentShaderSource(), "fragment");
      linkedProgram = createProgram();
      attachShader(linkedProgram, vertex);
      attachShader(linkedProgram, fragment);
      linkProgram(linkedProgram);
      if (getProgrami(linkedProgram, org.lwjgl.opengl.GL20.GL_LINK_STATUS) == 0) {
        throw new IllegalStateException(
            "OpenGL texture compositor link failed: " + getProgramInfoLog(linkedProgram));
      }
      return linkedProgram;
    } catch (RuntimeException error) {
      if (linkedProgram != 0) {
        deleteProgram(linkedProgram);
      }
      throw error;
    } finally {
      if (linkedProgram != 0 && vertex != 0) {
        detachShader(linkedProgram, vertex);
      }
      if (linkedProgram != 0 && fragment != 0) {
        detachShader(linkedProgram, fragment);
      }
      if (vertex != 0) {
        deleteShader(vertex);
      }
      if (fragment != 0) {
        deleteShader(fragment);
      }
    }
  }

  private int compileShader(int kind, String source, String name) {
    var shader = createShader(kind);
    shaderSource(shader, source);
    compileShader(shader);
    if (getShaderi(shader, org.lwjgl.opengl.GL20.GL_COMPILE_STATUS) == 0) {
      var log = getShaderInfoLog(shader);
      deleteShader(shader);
      throw new IllegalStateException(
          "OpenGL texture compositor %s shader compile failed: %s".formatted(name, log));
    }
    return shader;
  }

  private String vertexShaderSource() {
    if (gles) {
      return """
      #version 300 es
      out vec2 out_uv;
      const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
      const vec2 uvs[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
      void main() {
        gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
        out_uv = uvs[gl_VertexID];
      }
      """;
    }
    return """
    #version 130
    out vec2 out_uv;
    vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
    void main() {
      gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
      out_uv = uvs[gl_VertexID];
    }
    """;
  }

  private String fragmentShaderSource() {
    if (gles) {
      return """
      #version 300 es
      precision mediump float;
      uniform sampler2D map_texture;
      in vec2 out_uv;
      out vec4 out_color;
      void main() {
        out_color = texture(map_texture, out_uv);
      }
      """;
    }
    return """
    #version 130
    uniform sampler2D map_texture;
    in vec2 out_uv;
    out vec4 out_color;
    void main() {
      out_color = texture(map_texture, out_uv);
    }
    """;
  }

  private void checkError(String operation) {
    var error = gles ? org.lwjgl.opengles.GLES20.glGetError() : org.lwjgl.opengl.GL11.glGetError();
    if (error != org.lwjgl.opengl.GL11.GL_NO_ERROR) {
      throw new IllegalStateException(
          "%s failed with OpenGL error 0x%x".formatted(operation, error));
    }
  }

  private int createShader(int kind) {
    return gles
        ? org.lwjgl.opengles.GLES20.glCreateShader(kind)
        : org.lwjgl.opengl.GL20.glCreateShader(kind);
  }

  private void shaderSource(int shader, String source) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glShaderSource(shader, source);
    } else {
      org.lwjgl.opengl.GL20.glShaderSource(shader, source);
    }
  }

  private void compileShader(int shader) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glCompileShader(shader);
    } else {
      org.lwjgl.opengl.GL20.glCompileShader(shader);
    }
  }

  private int getShaderi(int shader, int pname) {
    return gles
        ? org.lwjgl.opengles.GLES20.glGetShaderi(shader, pname)
        : org.lwjgl.opengl.GL20.glGetShaderi(shader, pname);
  }

  private String getShaderInfoLog(int shader) {
    return gles
        ? org.lwjgl.opengles.GLES20.glGetShaderInfoLog(shader)
        : org.lwjgl.opengl.GL20.glGetShaderInfoLog(shader);
  }

  private void deleteShader(int shader) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glDeleteShader(shader);
    } else {
      org.lwjgl.opengl.GL20.glDeleteShader(shader);
    }
  }

  private int createProgram() {
    return gles
        ? org.lwjgl.opengles.GLES20.glCreateProgram()
        : org.lwjgl.opengl.GL20.glCreateProgram();
  }

  private void attachShader(int program, int shader) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glAttachShader(program, shader);
    } else {
      org.lwjgl.opengl.GL20.glAttachShader(program, shader);
    }
  }

  private void detachShader(int program, int shader) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glDetachShader(program, shader);
    } else {
      org.lwjgl.opengl.GL20.glDetachShader(program, shader);
    }
  }

  private void linkProgram(int program) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glLinkProgram(program);
    } else {
      org.lwjgl.opengl.GL20.glLinkProgram(program);
    }
  }

  private int getProgrami(int program, int pname) {
    return gles
        ? org.lwjgl.opengles.GLES20.glGetProgrami(program, pname)
        : org.lwjgl.opengl.GL20.glGetProgrami(program, pname);
  }

  private String getProgramInfoLog(int program) {
    return gles
        ? org.lwjgl.opengles.GLES20.glGetProgramInfoLog(program)
        : org.lwjgl.opengl.GL20.glGetProgramInfoLog(program);
  }

  private void deleteProgram(int program) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glDeleteProgram(program);
    } else {
      org.lwjgl.opengl.GL20.glDeleteProgram(program);
    }
  }

  private int genVertexArray() {
    return gles
        ? org.lwjgl.opengles.GLES30.glGenVertexArrays()
        : org.lwjgl.opengl.GL30.glGenVertexArrays();
  }

  private void bindVertexArray(int vertexArray) {
    if (gles) {
      org.lwjgl.opengles.GLES30.glBindVertexArray(vertexArray);
    } else {
      org.lwjgl.opengl.GL30.glBindVertexArray(vertexArray);
    }
  }

  private void deleteVertexArray(int vertexArray) {
    if (gles) {
      org.lwjgl.opengles.GLES30.glDeleteVertexArrays(vertexArray);
    } else {
      org.lwjgl.opengl.GL30.glDeleteVertexArrays(vertexArray);
    }
  }

  private void useProgram(int program) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glUseProgram(program);
    } else {
      org.lwjgl.opengl.GL20.glUseProgram(program);
    }
  }

  private int getUniformLocation(int program, String name) {
    return gles
        ? org.lwjgl.opengles.GLES20.glGetUniformLocation(program, name)
        : org.lwjgl.opengl.GL20.glGetUniformLocation(program, name);
  }

  private void uniform1i(int location, int value) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glUniform1i(location, value);
    } else {
      org.lwjgl.opengl.GL20.glUniform1i(location, value);
    }
  }

  private void bindFramebuffer(int framebuffer) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glBindFramebuffer(
          org.lwjgl.opengl.GL30.GL_FRAMEBUFFER, framebuffer);
    } else {
      org.lwjgl.opengl.GL30.glBindFramebuffer(org.lwjgl.opengl.GL30.GL_FRAMEBUFFER, framebuffer);
    }
  }

  private void disable(int capability) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glDisable(capability);
    } else {
      org.lwjgl.opengl.GL11.glDisable(capability);
    }
  }

  private void viewport(int x, int y, int width, int height) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glViewport(x, y, width, height);
    } else {
      org.lwjgl.opengl.GL11.glViewport(x, y, width, height);
    }
  }

  private void clearColor(float red, float green, float blue, float alpha) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glClearColor(red, green, blue, alpha);
    } else {
      org.lwjgl.opengl.GL11.glClearColor(red, green, blue, alpha);
    }
  }

  private void clear(int mask) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glClear(mask);
    } else {
      org.lwjgl.opengl.GL11.glClear(mask);
    }
  }

  private void activeTexture(int texture) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glActiveTexture(texture);
    } else {
      org.lwjgl.opengl.GL13.glActiveTexture(texture);
    }
  }

  private void bindTexture(int target, int texture) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glBindTexture(target, texture);
    } else {
      org.lwjgl.opengl.GL11.glBindTexture(target, texture);
    }
  }

  private void texParameteri(int target, int pname, int value) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glTexParameteri(target, pname, value);
    } else {
      org.lwjgl.opengl.GL11.glTexParameteri(target, pname, value);
    }
  }

  private void drawArrays(int mode, int first, int count) {
    if (gles) {
      org.lwjgl.opengles.GLES20.glDrawArrays(mode, first, count);
    } else {
      org.lwjgl.opengl.GL11.glDrawArrays(mode, first, count);
    }
  }
}
