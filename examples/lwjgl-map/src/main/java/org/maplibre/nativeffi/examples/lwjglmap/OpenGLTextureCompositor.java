package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.opengl.GL11.GL_COLOR_BUFFER_BIT;
import static org.lwjgl.opengl.GL11.GL_FALSE;
import static org.lwjgl.opengl.GL11.GL_LINEAR;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_MAG_FILTER;
import static org.lwjgl.opengl.GL11.GL_TEXTURE_MIN_FILTER;
import static org.lwjgl.opengl.GL11.GL_TRIANGLES;
import static org.lwjgl.opengl.GL11.glBindTexture;
import static org.lwjgl.opengl.GL11.glClear;
import static org.lwjgl.opengl.GL11.glClearColor;
import static org.lwjgl.opengl.GL11.glDrawArrays;
import static org.lwjgl.opengl.GL11.glTexParameteri;
import static org.lwjgl.opengl.GL11.glViewport;
import static org.lwjgl.opengl.GL13.GL_TEXTURE0;
import static org.lwjgl.opengl.GL13.glActiveTexture;
import static org.lwjgl.opengl.GL20.GL_COMPILE_STATUS;
import static org.lwjgl.opengl.GL20.GL_FRAGMENT_SHADER;
import static org.lwjgl.opengl.GL20.GL_LINK_STATUS;
import static org.lwjgl.opengl.GL20.GL_VERTEX_SHADER;
import static org.lwjgl.opengl.GL20.glAttachShader;
import static org.lwjgl.opengl.GL20.glCompileShader;
import static org.lwjgl.opengl.GL20.glCreateProgram;
import static org.lwjgl.opengl.GL20.glCreateShader;
import static org.lwjgl.opengl.GL20.glDeleteProgram;
import static org.lwjgl.opengl.GL20.glDeleteShader;
import static org.lwjgl.opengl.GL20.glGetProgramInfoLog;
import static org.lwjgl.opengl.GL20.glGetProgrami;
import static org.lwjgl.opengl.GL20.glGetShaderInfoLog;
import static org.lwjgl.opengl.GL20.glGetShaderi;
import static org.lwjgl.opengl.GL20.glGetUniformLocation;
import static org.lwjgl.opengl.GL20.glLinkProgram;
import static org.lwjgl.opengl.GL20.glShaderSource;
import static org.lwjgl.opengl.GL20.glUniform1i;
import static org.lwjgl.opengl.GL20.glUseProgram;
import static org.lwjgl.opengl.GL30.glBindVertexArray;
import static org.lwjgl.opengl.GL30.glDeleteVertexArrays;
import static org.lwjgl.opengl.GL30.glGenVertexArrays;

import org.maplibre.nativeffi.render.OpenGLOwnedTextureFrameHandle;

final class OpenGLTextureCompositor implements AutoCloseable {
  private static final String VERTEX_SHADER =
      """
      #version 130
      out vec2 out_uv;
      vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
      vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
      void main() {
        gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
        out_uv = uvs[gl_VertexID];
      }
      """;
  private static final String FRAGMENT_SHADER =
      """
      #version 130
      uniform sampler2D map_texture;
      in vec2 out_uv;
      out vec4 out_color;
      void main() {
        out_color = texture(map_texture, out_uv);
      }
      """;

  private final OpenGLContext context;
  private int program;
  private int vertexArray;
  private Viewport viewport;

  OpenGLTextureCompositor(OpenGLContext context, Viewport viewport) {
    this.context = context;
    this.viewport = viewport;
    context.makeCurrent();
    program = createProgram();
    vertexArray = glGenVertexArrays();
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "map_texture"), 0);
    glUseProgram(0);
  }

  void resize(Viewport viewport) {
    this.viewport = viewport;
  }

  void draw(OpenGLOwnedTextureFrameHandle frameHandle) {
    var frame = frameHandle.frame();
    if (frame.texture() == 0 || frame.width() <= 0 || frame.height() <= 0) {
      throw new IllegalStateException("MapLibre returned an empty OpenGL owned texture frame");
    }
    draw(frame.target(), frame.texture());
  }

  void draw(int target, int texture) {
    if (texture == 0) {
      throw new IllegalStateException("OpenGL texture name is 0");
    }
    context.makeCurrent();
    glViewport(0, 0, viewport.framebufferWidth(), viewport.framebufferHeight());
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);
    glBindVertexArray(vertexArray);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, texture);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindTexture(target, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    context.swapBuffers();
  }

  @Override
  public void close() {
    context.makeCurrent();
    if (vertexArray != 0) {
      glDeleteVertexArrays(vertexArray);
      vertexArray = 0;
    }
    if (program != 0) {
      glDeleteProgram(program);
      program = 0;
    }
  }

  private static int createProgram() {
    var vertex = compileShader(GL_VERTEX_SHADER, VERTEX_SHADER, "fullscreen vertex shader");
    var fragment = compileShader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER, "texture fragment shader");
    try {
      var program = glCreateProgram();
      glAttachShader(program, vertex);
      glAttachShader(program, fragment);
      glLinkProgram(program);
      if (glGetProgrami(program, GL_LINK_STATUS) == GL_FALSE) {
        var log = glGetProgramInfoLog(program);
        glDeleteProgram(program);
        throw new IllegalStateException("OpenGL compositor program link failed: " + log);
      }
      return program;
    } finally {
      glDeleteShader(fragment);
      glDeleteShader(vertex);
    }
  }

  private static int compileShader(int kind, String source, String name) {
    var shader = glCreateShader(kind);
    glShaderSource(shader, source);
    glCompileShader(shader);
    if (glGetShaderi(shader, GL_COMPILE_STATUS) == GL_FALSE) {
      var log = glGetShaderInfoLog(shader);
      glDeleteShader(shader);
      throw new IllegalStateException("OpenGL compositor " + name + " compile failed: " + log);
    }
    return shader;
  }
}
