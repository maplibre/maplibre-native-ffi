package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.opengl.GL11
import org.lwjgl.opengl.GL13
import org.lwjgl.opengl.GL20
import org.lwjgl.opengl.GL30
import org.lwjgl.opengles.GLES20
import org.lwjgl.opengles.GLES30

internal class OpenGLTextureCompositor(
  graphicsContext: GraphicsContext,
  private var viewport: Viewport,
) : AutoCloseable {
  private val context = graphicsContext as OpenGLContext
  private val gles = context.isGles
  private var program = 0
  private var vertexArray = 0

  init {
    context.makeCurrent()
    try {
      program = createTextureProgram()
      vertexArray = genVertexArray()
      useProgram(program)
      val sampler = getUniformLocation(program, "map_texture")
      if (sampler >= 0) {
        uniform1i(sampler, 0)
      }
      useProgram(0)
      checkError("initialize OpenGL texture compositor")
    } catch (error: RuntimeException) {
      close()
      throw error
    }
  }

  fun resize(viewport: Viewport) {
    this.viewport = viewport
  }

  fun drawTexture(texture: Int) {
    check(texture != 0) { "OpenGL texture name is zero" }
    context.makeCurrent()
    bindFramebuffer(0)
    disable(GL11.GL_CULL_FACE)
    disable(GL11.GL_DEPTH_TEST)
    disable(GL11.GL_SCISSOR_TEST)
    viewport(0, 0, viewport.framebufferWidth(), viewport.framebufferHeight())
    clearColor(0.08f, 0.09f, 0.11f, 1.0f)
    clear(GL11.GL_COLOR_BUFFER_BIT)
    useProgram(program)
    bindVertexArray(vertexArray)
    activeTexture(GL13.GL_TEXTURE0)
    bindTexture(TEXTURE_TARGET, texture)
    texParameteri(TEXTURE_TARGET, GL11.GL_TEXTURE_MIN_FILTER, GL11.GL_LINEAR)
    texParameteri(TEXTURE_TARGET, GL11.GL_TEXTURE_MAG_FILTER, GL11.GL_LINEAR)
    drawArrays(GL11.GL_TRIANGLES, 0, 3)
    bindTexture(TEXTURE_TARGET, 0)
    bindVertexArray(0)
    useProgram(0)
    checkError("draw OpenGL texture")
    context.swapBuffers()
  }

  override fun close() {
    try {
      context.makeCurrent()
    } catch (_: RuntimeException) {
      return
    }
    if (vertexArray != 0) {
      deleteVertexArray(vertexArray)
      vertexArray = 0
    }
    if (program != 0) {
      deleteProgram(program)
      program = 0
    }
  }

  private fun createTextureProgram(): Int {
    val vertex = compileShader(GL20.GL_VERTEX_SHADER, vertexShaderSource(), "vertex")
    var fragment = 0
    var linkedProgram = 0
    try {
      fragment = compileShader(GL20.GL_FRAGMENT_SHADER, fragmentShaderSource(), "fragment")
      linkedProgram = createProgram()
      attachShader(linkedProgram, vertex)
      attachShader(linkedProgram, fragment)
      linkProgram(linkedProgram)
      if (getProgrami(linkedProgram, GL20.GL_LINK_STATUS) == 0) {
        error("OpenGL texture compositor link failed: ${getProgramInfoLog(linkedProgram)}")
      }
      return linkedProgram
    } catch (error: RuntimeException) {
      if (linkedProgram != 0) {
        deleteProgram(linkedProgram)
      }
      throw error
    } finally {
      if (linkedProgram != 0 && vertex != 0) {
        detachShader(linkedProgram, vertex)
      }
      if (linkedProgram != 0 && fragment != 0) {
        detachShader(linkedProgram, fragment)
      }
      if (vertex != 0) {
        deleteShader(vertex)
      }
      if (fragment != 0) {
        deleteShader(fragment)
      }
    }
  }

  private fun compileShader(kind: Int, source: String, name: String): Int {
    val shader = createShader(kind)
    shaderSource(shader, source)
    compileShader(shader)
    if (getShaderi(shader, GL20.GL_COMPILE_STATUS) == 0) {
      val log = getShaderInfoLog(shader)
      deleteShader(shader)
      error("OpenGL texture compositor $name shader compile failed: $log")
    }
    return shader
  }

  private fun vertexShaderSource(): String =
    if (gles) {
      """
      #version 300 es
      out vec2 out_uv;
      const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
      const vec2 uvs[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
      void main() {
        gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
        out_uv = uvs[gl_VertexID];
      }
      """
        .trimIndent()
    } else {
      """
      #version 130
      out vec2 out_uv;
      vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
      vec2 uvs[3] = vec2[](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));
      void main() {
        gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
        out_uv = uvs[gl_VertexID];
      }
      """
        .trimIndent()
    }

  private fun fragmentShaderSource(): String =
    if (gles) {
      """
      #version 300 es
      precision mediump float;
      uniform sampler2D map_texture;
      in vec2 out_uv;
      out vec4 out_color;
      void main() {
        out_color = texture(map_texture, out_uv);
      }
      """
        .trimIndent()
    } else {
      """
      #version 130
      uniform sampler2D map_texture;
      in vec2 out_uv;
      out vec4 out_color;
      void main() {
        out_color = texture(map_texture, out_uv);
      }
      """
        .trimIndent()
    }

  private fun checkError(operation: String) {
    val error = if (gles) GLES20.glGetError() else GL11.glGetError()
    if (error != GL11.GL_NO_ERROR) {
      throw IllegalStateException("%s failed with OpenGL error 0x%x".format(operation, error))
    }
  }

  private fun createShader(kind: Int): Int =
    if (gles) GLES20.glCreateShader(kind) else GL20.glCreateShader(kind)

  private fun shaderSource(shader: Int, source: String) {
    if (gles) {
      GLES20.glShaderSource(shader, source)
    } else {
      GL20.glShaderSource(shader, source)
    }
  }

  private fun compileShader(shader: Int) {
    if (gles) {
      GLES20.glCompileShader(shader)
    } else {
      GL20.glCompileShader(shader)
    }
  }

  private fun getShaderi(shader: Int, pname: Int): Int =
    if (gles) GLES20.glGetShaderi(shader, pname) else GL20.glGetShaderi(shader, pname)

  private fun getShaderInfoLog(shader: Int): String =
    if (gles) GLES20.glGetShaderInfoLog(shader) else GL20.glGetShaderInfoLog(shader)

  private fun deleteShader(shader: Int) {
    if (gles) {
      GLES20.glDeleteShader(shader)
    } else {
      GL20.glDeleteShader(shader)
    }
  }

  private fun createProgram(): Int = if (gles) GLES20.glCreateProgram() else GL20.glCreateProgram()

  private fun attachShader(program: Int, shader: Int) {
    if (gles) {
      GLES20.glAttachShader(program, shader)
    } else {
      GL20.glAttachShader(program, shader)
    }
  }

  private fun detachShader(program: Int, shader: Int) {
    if (gles) {
      GLES20.glDetachShader(program, shader)
    } else {
      GL20.glDetachShader(program, shader)
    }
  }

  private fun linkProgram(program: Int) {
    if (gles) {
      GLES20.glLinkProgram(program)
    } else {
      GL20.glLinkProgram(program)
    }
  }

  private fun getProgrami(program: Int, pname: Int): Int =
    if (gles) GLES20.glGetProgrami(program, pname) else GL20.glGetProgrami(program, pname)

  private fun getProgramInfoLog(program: Int): String =
    if (gles) GLES20.glGetProgramInfoLog(program) else GL20.glGetProgramInfoLog(program)

  private fun deleteProgram(program: Int) {
    if (gles) {
      GLES20.glDeleteProgram(program)
    } else {
      GL20.glDeleteProgram(program)
    }
  }

  private fun genVertexArray(): Int =
    if (gles) GLES30.glGenVertexArrays() else GL30.glGenVertexArrays()

  private fun bindVertexArray(vertexArray: Int) {
    if (gles) {
      GLES30.glBindVertexArray(vertexArray)
    } else {
      GL30.glBindVertexArray(vertexArray)
    }
  }

  private fun deleteVertexArray(vertexArray: Int) {
    if (gles) {
      GLES30.glDeleteVertexArrays(vertexArray)
    } else {
      GL30.glDeleteVertexArrays(vertexArray)
    }
  }

  private fun useProgram(program: Int) {
    if (gles) {
      GLES20.glUseProgram(program)
    } else {
      GL20.glUseProgram(program)
    }
  }

  private fun getUniformLocation(program: Int, name: String): Int =
    if (gles) GLES20.glGetUniformLocation(program, name)
    else GL20.glGetUniformLocation(program, name)

  private fun uniform1i(location: Int, value: Int) {
    if (gles) {
      GLES20.glUniform1i(location, value)
    } else {
      GL20.glUniform1i(location, value)
    }
  }

  private fun bindFramebuffer(framebuffer: Int) {
    if (gles) {
      GLES20.glBindFramebuffer(GL30.GL_FRAMEBUFFER, framebuffer)
    } else {
      GL30.glBindFramebuffer(GL30.GL_FRAMEBUFFER, framebuffer)
    }
  }

  private fun disable(capability: Int) {
    if (gles) {
      GLES20.glDisable(capability)
    } else {
      GL11.glDisable(capability)
    }
  }

  private fun viewport(x: Int, y: Int, width: Int, height: Int) {
    if (gles) {
      GLES20.glViewport(x, y, width, height)
    } else {
      GL11.glViewport(x, y, width, height)
    }
  }

  private fun clearColor(red: Float, green: Float, blue: Float, alpha: Float) {
    if (gles) {
      GLES20.glClearColor(red, green, blue, alpha)
    } else {
      GL11.glClearColor(red, green, blue, alpha)
    }
  }

  private fun clear(mask: Int) {
    if (gles) {
      GLES20.glClear(mask)
    } else {
      GL11.glClear(mask)
    }
  }

  private fun activeTexture(texture: Int) {
    if (gles) {
      GLES20.glActiveTexture(texture)
    } else {
      GL13.glActiveTexture(texture)
    }
  }

  private fun bindTexture(target: Int, texture: Int) {
    if (gles) {
      GLES20.glBindTexture(target, texture)
    } else {
      GL11.glBindTexture(target, texture)
    }
  }

  private fun texParameteri(target: Int, pname: Int, value: Int) {
    if (gles) {
      GLES20.glTexParameteri(target, pname, value)
    } else {
      GL11.glTexParameteri(target, pname, value)
    }
  }

  private fun drawArrays(mode: Int, first: Int, count: Int) {
    if (gles) {
      GLES20.glDrawArrays(mode, first, count)
    } else {
      GL11.glDrawArrays(mode, first, count)
    }
  }

  internal companion object {
    const val TEXTURE_TARGET: Int = GL11.GL_TEXTURE_2D
  }
}
