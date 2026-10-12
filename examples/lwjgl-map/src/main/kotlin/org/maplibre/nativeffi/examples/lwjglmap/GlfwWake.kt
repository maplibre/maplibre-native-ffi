package org.maplibre.nativeffi.examples.lwjglmap

import java.util.concurrent.atomic.AtomicBoolean
import org.lwjgl.glfw.GLFW.glfwPostEmptyEvent
import org.maplibre.nativeffi.generated.Wake

/**
 * A native wake for the GLFW thread. The wake marks its work and posts an empty GLFW event, so it
 * returns at once and the GLFW thread does the work after its wait ends.
 */
internal class GlfwWake {
  private val raised = AtomicBoolean(false)

  val wake = Wake {
    raised.set(true)
    glfwPostEmptyEvent()
  }

  /** Reports whether the wake fired since the last call. */
  fun consume(): Boolean = raised.getAndSet(false)
}
