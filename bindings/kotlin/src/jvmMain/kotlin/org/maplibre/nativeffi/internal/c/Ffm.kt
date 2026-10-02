package org.maplibre.nativeffi.internal.c

import java.lang.foreign.Arena
import java.lang.foreign.FunctionDescriptor
import java.lang.foreign.GroupLayout
import java.lang.foreign.Linker
import java.lang.foreign.MemoryLayout
import java.lang.foreign.MemorySegment
import java.lang.foreign.SegmentAllocator
import java.lang.foreign.SymbolLookup
import java.lang.foreign.ValueLayout
import java.lang.invoke.MethodHandle
import java.lang.invoke.MethodHandles
import java.lang.invoke.MethodType

/**
 * Builds the FFM downcalls and upcall stubs that the generated declarations name.
 *
 * Pointers cross as `JAVA_LONG`: every JVM target is 64-bit, where a pointer and a 64-bit integer
 * share one calling convention. A record passed or returned by value keeps its full layout, which
 * the calling convention classifies field by field.
 */
internal object Ffm {
  private val linker = Linker.nativeLinker()
  private val symbols = SymbolLookup.loaderLookup()
  private val segmentAddress =
    MethodHandles.lookup()
      .findVirtual(
        MemorySegment::class.java,
        "address",
        MethodType.methodType(Long::class.javaPrimitiveType),
      )

  /** The downcall for the C function [name], returning [result] or void when it is null. */
  fun downcall(name: String, result: MemoryLayout?, vararg arguments: MemoryLayout): MethodHandle =
    linker.downcallHandle(symbols.findOrThrow(name), descriptor(result, arguments))

  /**
   * An upcall stub that calls the static method [name] of [Upcalls] and lives for the process.
   *
   * The method takes each record argument as the address of the caller's copy.
   */
  fun upcall(name: String, result: MemoryLayout?, vararg arguments: MemoryLayout): Long {
    val type = MethodType.methodType(result?.let(::carrier) ?: Void.TYPE, arguments.map(::carrier))
    var target = MethodHandles.lookup().findStatic(Upcalls::class.java, name, type)
    arguments.forEachIndexed { index, layout ->
      if (layout is GroupLayout)
        target = MethodHandles.filterArguments(target, index, segmentAddress)
    }
    return linker.upcallStub(target, descriptor(result, arguments), Arena.global()).address()
  }

  /** A record layout from its fields, with the padding the generator computed. */
  fun struct(vararg fields: MemoryLayout): GroupLayout = MemoryLayout.structLayout(*fields)

  fun union(vararg fields: MemoryLayout): GroupLayout = MemoryLayout.unionLayout(*fields)

  fun pad(size: Long): MemoryLayout = MemoryLayout.paddingLayout(size)

  fun array(count: Long, element: MemoryLayout): MemoryLayout =
    MemoryLayout.sequenceLayout(count, element)

  /** The record of [layout] at [address], for a by-value argument. */
  fun value(address: Long, layout: MemoryLayout): MemorySegment =
    MemorySegment.ofAddress(address).reinterpret(layout.byteSize())

  /** Writes a by-value result straight to [address]. */
  fun into(address: Long, layout: MemoryLayout): SegmentAllocator =
    SegmentAllocator.prefixAllocator(value(address, layout))

  private fun descriptor(result: MemoryLayout?, arguments: Array<out MemoryLayout>) =
    if (result == null) FunctionDescriptor.ofVoid(*arguments)
    else FunctionDescriptor.of(result, *arguments)

  private fun carrier(layout: MemoryLayout): Class<*> =
    if (layout is ValueLayout) layout.carrier() else Long::class.javaPrimitiveType!!
}
