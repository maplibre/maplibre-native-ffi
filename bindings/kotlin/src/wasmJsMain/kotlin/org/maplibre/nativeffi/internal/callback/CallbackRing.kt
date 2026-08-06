package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.generated.MlnKotlinRecord
import org.maplibre.nativeffi.internal.wasm.generated.MlnKotlinRecordKind
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_dropped_records
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_set_wake
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_take_record

/** One custom geometry source's tile callbacks, as the ring reaches them. */
internal interface RingTileCallbacks {
  fun tile(tileId: CanonicalTileId, cancelled: Boolean)

  /** Reports that native raises no further tile callback for this registration. */
  fun retired()
}

/**
 * The one path from a MapLibre thread into this binding.
 *
 * A JavaScript function belongs to the agent that defined it, so no MapLibre thread may enter this
 * WebAssembly instance. The C shim copies each callback into a bounded ring instead, and this
 * drains the ring inside `pump`. A retirement marker travels in the same ring, behind the records
 * it retires.
 */
internal object CallbackRing {
  /** The tile z that `mln_adapter_custom_geometry_callbacks_retire` marks a retirement with. */
  private const val RETIREMENT_TILE_Z = 255

  private val tileCallbacks = mutableMapOf<Int, RingTileCallbacks>()
  private var nextTileToken = 1
  private var wake = 0L

  /** How many records the ring dropped because a host stopped draining it, cumulative. */
  val droppedRecords: Long
    get() = mln_kotlin_dropped_records()

  /** Delivers every queued record, oldest first. */
  fun drain() {
    Heap.withScratch(MlnKotlinRecord.SIZEOF) { record ->
      while (mln_kotlin_take_record(record.address) != 0) {
        // Contained, because a record that fails to decode must not strand the ones behind it, and
        // the pump this runs inside is nobody's callback to report to. The branch that takes a
        // payload releases it whatever its delivery does.
        runCatching { deliver(record) }
      }
    }
  }

  /**
   * Names the wake source that a producing thread signals after queueing a record.
   *
   * One source at a time, so with two runtimes a record releases the newer one's parked pump and
   * the other returns on its own timeout.
   */
  fun setWake(source: Long) {
    wake = source
    mln_kotlin_set_wake(source)
  }

  /** Clears [source] if it is the one installed, before the runtime that owns it destroys it. */
  fun clearWake(source: Long) {
    if (wake != source) return
    wake = 0
    mln_kotlin_set_wake(0)
  }

  /** Registers [callbacks] and returns the `user_data` to register natively for them. */
  fun addTileCallbacks(callbacks: RingTileCallbacks): HeapPointer {
    // A token rather than an address, because native carries this value back unread. Counting from
    // one keeps it distinguishable from a null user_data.
    val token = nextTileToken
    nextTileToken += 1
    tileCallbacks[token] = callbacks
    return HeapPointer(token)
  }

  private fun deliver(record: HeapPointer) {
    val payload = MlnKotlinRecord.payload(record)
    when (MlnKotlinRecord.kind(record)) {
      MlnKotlinRecordKind.MLN_KOTLIN_RECORD_LOG -> LogCallbackState.deliver(payload)
      MlnKotlinRecordKind.MLN_KOTLIN_RECORD_LOG_RETIRED -> LogCallbackState.retired()
      MlnKotlinRecordKind.MLN_KOTLIN_RECORD_RESOURCE_REQUEST ->
        QueuedResourceProviders.deliver(payload)
      MlnKotlinRecordKind.MLN_KOTLIN_RECORD_RESOURCE_PROVIDER_RETIRED ->
        QueuedResourceProviders.retired()
      MlnKotlinRecordKind.MLN_KOTLIN_RECORD_TILE_FETCH -> tile(record, payload, cancelled = false)
      MlnKotlinRecordKind.MLN_KOTLIN_RECORD_TILE_CANCEL -> tile(record, payload, cancelled = true)
    }
  }

  private fun tile(record: HeapPointer, userData: HeapPointer, cancelled: Boolean) {
    val z = MlnKotlinRecord.tileZ(record)
    if (z == RETIREMENT_TILE_Z) {
      // Retirement invokes both callbacks once, so the second one finds the entry already gone.
      tileCallbacks.remove(userData.address)?.retired()
      return
    }
    val callbacks = tileCallbacks[userData.address] ?: return
    // Unsigned in C and signed here, so they widen through their bit pattern into the Long the
    // public type carries the whole unsigned domain in.
    val tileId =
      CanonicalTileId(
        z,
        MlnKotlinRecord.tileX(record).toUInt().toLong(),
        MlnKotlinRecord.tileY(record).toUInt().toLong(),
      )
    callbacks.tile(tileId, cancelled)
  }
}
