package org.maplibre.nativeffi.render

import java.lang.foreign.Arena
import java.lang.foreign.FunctionDescriptor
import java.lang.foreign.Linker
import java.lang.foreign.MemorySegment
import java.lang.foreign.SymbolLookup
import java.lang.foreign.ValueLayout.ADDRESS
import org.maplibre.nativeffi.generated.MetalContextDescriptor
import org.maplibre.nativeffi.generated.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.generated.RenderTargetExtent
import org.maplibre.nativeffi.map.MapHandle

internal fun attachJvmMetal(
  map: MapHandle,
  width: Int,
  height: Int,
  depth: UInt,
): OwnedTextureTestSession {
  val arena = Arena.ofShared()
  try {
    val linker = Linker.nativeLinker()
    val metal =
      SymbolLookup.libraryLookup("/System/Library/Frameworks/Metal.framework/Metal", arena)
    val objc = SymbolLookup.libraryLookup("/usr/lib/libobjc.A.dylib", arena)
    val create =
      linker.downcallHandle(
        metal.find("MTLCreateSystemDefaultDevice").orElseThrow(),
        FunctionDescriptor.of(ADDRESS),
      )
    val release =
      linker.downcallHandle(
        objc.find("objc_release").orElseThrow(),
        FunctionDescriptor.ofVoid(ADDRESS),
      )
    val device = create.invokeWithArguments() as MemorySegment
    check(device.address() != 0L) { "MTLCreateSystemDefaultDevice returned nil" }
    return attachOwnedTextureFixture(
      map,
      width,
      height,
      depth,
      attach = { target, w, h, options ->
        target.metalOwnedTextureAttach(
          MetalOwnedTextureDescriptor(
            RenderTargetExtent(w.toUInt(), h.toUInt(), 1.0),
            MetalContextDescriptor(NativePointer.ofAddress(device.address())),
          ),
          options,
        )
      },
      frameSize = { frame ->
        frame.withGetMetalTexture { OwnedTextureFrameSize(it.width.toInt(), it.height.toInt()) }
      },
      releaseGraphics = {
        release.invokeWithArguments(device)
        arena.close()
      },
    )
  } catch (error: Throwable) {
    if (arena.scope().isAlive) arena.close()
    throw error
  }
}
