package org.maplibre.nativeffi.examples.composemap.surface

import java.awt.Desktop
import java.util.concurrent.atomic.AtomicBoolean
import javax.swing.SwingUtilities

internal object DesktopNativeRenderingLifecycle {
  private val lock = Any()
  private val participants = LinkedHashMap<Long, Participant>()
  private var nextId = 1L
  private var quitHandlerInstalled = false
  private var quitHandlerUnavailable = false

  fun register(closeAction: () -> Unit): NativeRenderingParticipant {
    installQuitHandler()
    return synchronized(lock) {
      val id = nextId++
      val participant = Participant(id, closeAction)
      participants[id] = participant
      participant
    }
  }

  private fun installQuitHandler() {
    synchronized(lock) {
      if (quitHandlerInstalled || quitHandlerUnavailable) {
        return
      }
      if (!Desktop.isDesktopSupported()) {
        quitHandlerUnavailable = true
        return
      }
      val desktop = Desktop.getDesktop()
      if (!desktop.isSupported(Desktop.Action.APP_QUIT_HANDLER)) {
        quitHandlerUnavailable = true
        return
      }
      desktop.setQuitHandler { _, response ->
        try {
          closeAll()
          response.performQuit()
        } catch (error: Throwable) {
          error.printStackTrace()
          response.cancelQuit()
        }
      }
      quitHandlerInstalled = true
    }
  }

  private fun closeAll() {
    val snapshot = synchronized(lock) { participants.values.toList().asReversed() }
    snapshot.forEach { it.close() }
    runOnEventDispatchThread { SkikoHost.close() }
  }

  private fun unregister(id: Long) {
    synchronized(lock) { participants.remove(id) }
  }

  private fun runOnEventDispatchThread(action: () -> Unit) {
    if (SwingUtilities.isEventDispatchThread()) {
      action()
    } else {
      SwingUtilities.invokeAndWait { action() }
    }
  }

  private class Participant(private val id: Long, private val closeAction: () -> Unit) :
    NativeRenderingParticipant {
    private val closed = AtomicBoolean(false)

    override fun close() {
      if (!closed.compareAndSet(false, true)) {
        return
      }
      try {
        DesktopNativeRenderingLifecycle.runOnEventDispatchThread(closeAction)
      } catch (error: Throwable) {
        closed.set(false)
        throw error
      }
      DesktopNativeRenderingLifecycle.unregister(id)
    }
  }
}

internal interface NativeRenderingParticipant : AutoCloseable {
  override fun close()
}
