from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import Metal
import Quartz

from maplibre_native_ffi import render


class MetalUnavailableError(RuntimeError):
    pass


def _addr(value: Any) -> int:
    try:
        return int(value.__c_void_p__().value or 0)
    except AttributeError:
        msg = f"{type(value)!r} cannot expose an Objective-C object pointer"
        raise MetalUnavailableError(msg) from None


def _pointer(value: Any, name: str) -> render.NativePointer:
    return render.NativePointer(_addr(value), _diagnostic_name=name)


@dataclass(slots=True)
class MetalContext:
    device: Any
    _closed: bool = False

    @classmethod
    def create(cls) -> "MetalContext":
        device = Metal.MTLCreateSystemDefaultDevice()
        if device is None:
            msg = "MTLCreateSystemDefaultDevice returned nil"
            raise MetalUnavailableError(msg)
        return cls(device=device)

    def descriptor(self) -> render.MetalContextDescriptor:
        return render.MetalContextDescriptor(device=_pointer(self.device, "MTLDevice"))

    def owned_texture_descriptor(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> render.MetalOwnedTextureDescriptor:
        return render.MetalOwnedTextureDescriptor(
            extent=render.RenderTargetExtent(width, height, scale_factor),
            context=self.descriptor(),
        )

    def surface(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> "MetalSurface":
        return MetalSurface.create(self, width, height, scale_factor)

    def borrowed_texture(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> "MetalBorrowedTexture":
        return MetalBorrowedTexture.create(self, width, height, scale_factor)

    def close(self) -> None:
        self._closed = True

    def __enter__(self) -> "MetalContext":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()


@dataclass(slots=True)
class MetalSurface:
    context: MetalContext
    layer: Any
    width: int
    height: int
    scale_factor: float
    _closed: bool = False

    @classmethod
    def create(
        cls,
        context: MetalContext,
        width: int,
        height: int,
        scale_factor: float,
    ) -> "MetalSurface":
        layer = Quartz.CAMetalLayer.layer()
        if layer is None:
            msg = "CAMetalLayer.layer returned nil"
            raise MetalUnavailableError(msg)
        layer.setDevice_(context.device)
        layer.setPixelFormat_(Metal.MTLPixelFormatBGRA8Unorm)
        layer.setFramebufferOnly_(False)
        layer.setDrawableSize_(Quartz.CGSizeMake(width, height))
        return cls(context, layer, width, height, scale_factor)

    def descriptor(self) -> render.MetalSurfaceDescriptor:
        return render.MetalSurfaceDescriptor(
            extent=render.RenderTargetExtent(
                self.width,
                self.height,
                self.scale_factor,
            ),
            context=self.context.descriptor(),
            layer=_pointer(self.layer, "CAMetalLayer"),
        )

    def close(self) -> None:
        self._closed = True

    def __enter__(self) -> "MetalSurface":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()


@dataclass(slots=True)
class MetalBorrowedTexture:
    context: MetalContext
    texture: Any
    width: int
    height: int
    scale_factor: float
    _closed: bool = False

    @classmethod
    def create(
        cls,
        context: MetalContext,
        width: int,
        height: int,
        scale_factor: float,
    ) -> "MetalBorrowedTexture":
        descriptor = Metal.MTLTextureDescriptor.texture2DDescriptorWithPixelFormat_width_height_mipmapped_(
            Metal.MTLPixelFormatRGBA8Unorm,
            width,
            height,
            False,
        )
        descriptor.setUsage_(
            Metal.MTLTextureUsageShaderRead | Metal.MTLTextureUsageRenderTarget
        )
        texture = context.device.newTextureWithDescriptor_(descriptor)
        if texture is None:
            msg = "MTLDevice.newTextureWithDescriptor returned nil"
            raise MetalUnavailableError(msg)
        # A new MTLTexture's contents are undefined, and readback tests need a
        # known starting value.
        blank = bytes(width * height * 4)
        texture.replaceRegion_mipmapLevel_withBytes_bytesPerRow_(
            Metal.MTLRegionMake2D(0, 0, width, height),
            0,
            blank,
            width * 4,
        )
        return cls(context, texture, width, height, scale_factor)

    def descriptor(self) -> render.MetalBorrowedTextureDescriptor:
        return render.MetalBorrowedTextureDescriptor(
            extent=render.RenderTargetExtent(
                self.width,
                self.height,
                self.scale_factor,
            ),
            physical_width=self.width,
            physical_height=self.height,
            texture=_pointer(self.texture, "MTLTexture"),
        )

    def read_rgba(self) -> bytes:
        queue = self.context.device.newCommandQueue()
        if queue is None:
            msg = "MTLDevice.newCommandQueue returned nil"
            raise MetalUnavailableError(msg)
        command_buffer = queue.commandBuffer()
        encoder = command_buffer.blitCommandEncoder()
        # A texture created without an explicit storage mode is managed on
        # macOS, so its CPU copy stays stale until a blit synchronizes it.
        encoder.synchronizeResource_(self.texture)
        encoder.endEncoding()
        command_buffer.commit()
        command_buffer.waitUntilCompleted()
        pixels = bytearray(self.width * self.height * 4)
        self.texture.getBytes_bytesPerRow_fromRegion_mipmapLevel_(
            pixels,
            self.width * 4,
            Metal.MTLRegionMake2D(0, 0, self.width, self.height),
            0,
        )
        return bytes(pixels)

    def exists(self) -> bool:
        return not self._closed and self.texture is not None

    def close(self) -> None:
        self._closed = True

    def __enter__(self) -> "MetalBorrowedTexture":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()
