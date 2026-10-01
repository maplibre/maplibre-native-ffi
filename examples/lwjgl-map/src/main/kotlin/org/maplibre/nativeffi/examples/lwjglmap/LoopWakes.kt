package org.maplibre.nativeffi.examples.lwjglmap

import java.util.concurrent.atomic.AtomicBoolean
import org.lwjgl.glfw.GLFW.glfwPostEmptyEvent
import org.maplibre.nativeffi.generated.RenderDriverKind
import org.maplibre.nativeffi.generated.RenderSessionAttachOptions
import org.maplibre.nativeffi.generated.Wake

/**
 * The native wakes that bring the GLFW thread back from its wait. Each one marks its receiver's
 * work and posts an empty GLFW event, so the wake returns at once and the GLFW thread does the
 * work.
 */
internal class LoopWakes {
  /** The runtime has events to drain. */
  val events = LoopWake()

  /** The render session has frame results to drain. */
  val frames = LoopWake()

  /** The render session has driver work for the GLFW thread. */
  val driverWork = LoopWake()

  /** Attach options for a session that the GLFW thread drives. */
  fun attachOptions(ringDepth: UInt = 0u): RenderSessionAttachOptions =
    RenderSessionAttachOptions(
      driver = RenderDriverKind.CALLER_GRAPHICS_THREAD,
      requestedTextureRingDepth = ringDepth,
      frameWake = frames.wake,
      driverWorkWake = driverWork.wake,
    )
}

internal class LoopWake {
  private val raised = AtomicBoolean(false)

  val wake = Wake {
    raised.set(true)
    glfwPostEmptyEvent()
  }

  /** Reports whether the wake fired since the last call. */
  fun consume(): Boolean = raised.getAndSet(false)
}
