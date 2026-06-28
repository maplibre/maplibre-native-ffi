from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any
import ctypes
import os
import sys


if sys.platform == "darwin":
    os.environ.setdefault("PYOPENGL_PLATFORM", "egl")
    _egl_root = Path(os.environ.get("MLN_FFI_EGL_ROOT", "")).expanduser()
    _cdll = ctypes.CDLL

    class _MacAngleCDLL(_cdll):  # type: ignore[misc]
        def __init__(self, name: Any, *args: Any, **kwargs: Any) -> None:
            name = _macos_angle_library_path(name)
            super().__init__(name, *args, **kwargs)

    def _macos_angle_library_path(name: Any) -> Any:
        if name in {"EGL", "GLESv2"} and _egl_root.is_dir():
            library_path = _egl_root / f"lib{name}.dylib"
            if library_path.is_file():
                return str(library_path)
        return name

    ctypes.CDLL = _MacAngleCDLL  # type: ignore[assignment]
    ctypes.cdll._dlltype = _MacAngleCDLL  # type: ignore[attr-defined]

from OpenGL import EGL
from OpenGL import GLES3 as GL

from maplibre_native import render


class EglUnavailableError(RuntimeError):
    pass


EGL_PLATFORM_ANGLE_ANGLE = 0x3202
EGL_PLATFORM_ANGLE_TYPE_ANGLE = 0x3203
EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE = 0x3209
EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE = 0x320A
EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE = 0x3489


def _addr(value: Any) -> int:
    return ctypes.cast(value, ctypes.c_void_p).value or 0


def _pointer(value: Any, name: str) -> render.NativePointer:
    return render.NativePointer(_addr(value), _diagnostic_name=name)


def _display() -> Any:
    if sys.platform != "darwin":
        return EGL.eglGetDisplay(EGL.EGL_DEFAULT_DISPLAY)

    attributes = [
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
        EGL.EGL_NONE,
    ]
    return EGL.eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, None, attributes)


@dataclass(slots=True)
class EglContext:
    display: Any
    config: Any
    context: Any
    _closed: bool = False

    @classmethod
    def create(cls) -> "EglContext":
        display = _display()
        if _addr(display) == 0:
            msg = "EGL display creation returned EGL_NO_DISPLAY"
            raise EglUnavailableError(msg)

        major = EGL.EGLint()
        minor = EGL.EGLint()
        if not EGL.eglInitialize(display, major, minor):
            msg = "eglInitialize failed"
            raise EglUnavailableError(msg)

        configs = (EGL.EGLConfig * 8)()
        config_count = EGL.EGLint()
        attributes = [
            EGL.EGL_SURFACE_TYPE,
            EGL.EGL_PBUFFER_BIT,
            EGL.EGL_RENDERABLE_TYPE,
            EGL.EGL_OPENGL_ES3_BIT,
            EGL.EGL_RED_SIZE,
            8,
            EGL.EGL_GREEN_SIZE,
            8,
            EGL.EGL_BLUE_SIZE,
            8,
            EGL.EGL_ALPHA_SIZE,
            8,
            EGL.EGL_DEPTH_SIZE,
            24,
            EGL.EGL_STENCIL_SIZE,
            8,
            EGL.EGL_NONE,
        ]
        if not EGL.eglChooseConfig(
            display,
            attributes,
            configs,
            len(configs),
            config_count,
        ):
            EGL.eglTerminate(display)
            msg = "eglChooseConfig failed"
            raise EglUnavailableError(msg)
        if config_count.value == 0:
            EGL.eglTerminate(display)
            msg = "no EGL OpenGL pbuffer config was found"
            raise EglUnavailableError(msg)

        if not EGL.eglBindAPI(EGL.EGL_OPENGL_ES_API):
            EGL.eglTerminate(display)
            msg = "eglBindAPI(EGL_OPENGL_ES_API) failed"
            raise EglUnavailableError(msg)

        context = EGL.eglCreateContext(
            display,
            configs[0],
            EGL.EGL_NO_CONTEXT,
            [
                EGL.EGL_CONTEXT_CLIENT_VERSION,
                3,
                EGL.EGL_NONE,
            ],
        )
        if _addr(context) == 0:
            EGL.eglTerminate(display)
            msg = "eglCreateContext failed"
            raise EglUnavailableError(msg)

        return cls(display=display, config=configs[0], context=context)

    def descriptor(self) -> render.EglContextDescriptor:
        return render.EglContextDescriptor(
            display=_pointer(self.display, "EGLDisplay"),
            config=_pointer(self.config, "EGLConfig"),
            share_context=_pointer(self.context, "EGLContext"),
            get_proc_address=render.NativePointer(
                EGL.eglGetProcAddress(b"eglGetProcAddress"),
                _diagnostic_name="eglGetProcAddress",
            ),
        )

    def owned_texture_descriptor(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> render.OpenGLOwnedTextureDescriptor:
        return render.OpenGLOwnedTextureDescriptor(
            extent=render.RenderTargetExtent(width, height, scale_factor),
            context=self.descriptor(),
        )

    def pbuffer_surface(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> "EglPbufferSurface":
        return EglPbufferSurface.create(self, width, height, scale_factor)

    def borrowed_texture(
        self,
        width: int = 64,
        height: int = 64,
        scale_factor: float = 1.0,
    ) -> "EglBorrowedTexture":
        return EglBorrowedTexture.create(self, width, height, scale_factor)

    def make_current(self, surface: "EglPbufferSurface") -> None:
        if not EGL.eglMakeCurrent(
            self.display,
            surface.surface,
            surface.surface,
            self.context,
        ):
            msg = "eglMakeCurrent failed"
            raise EglUnavailableError(msg)

    def clear_current(self) -> None:
        EGL.eglMakeCurrent(
            self.display,
            EGL.EGL_NO_SURFACE,
            EGL.EGL_NO_SURFACE,
            EGL.EGL_NO_CONTEXT,
        )

    def close(self) -> None:
        if self._closed:
            return
        self.clear_current()
        EGL.eglDestroyContext(self.display, self.context)
        EGL.eglTerminate(self.display)
        self._closed = True

    def __enter__(self) -> "EglContext":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()


@dataclass(slots=True)
class EglBorrowedTexture:
    context: EglContext
    surface: EglPbufferSurface
    texture: int
    width: int
    height: int
    scale_factor: float
    _closed: bool = False

    @classmethod
    def create(
        cls,
        context: EglContext,
        width: int,
        height: int,
        scale_factor: float,
    ) -> "EglBorrowedTexture":
        surface = context.pbuffer_surface(width, height, scale_factor)
        try:
            context.make_current(surface)
            texture = int(GL.glGenTextures(1))
            if texture == 0:
                msg = "glGenTextures returned 0"
                raise EglUnavailableError(msg)
            GL.glBindTexture(GL.GL_TEXTURE_2D, texture)
            GL.glTexParameteri(
                GL.GL_TEXTURE_2D,
                GL.GL_TEXTURE_MIN_FILTER,
                GL.GL_LINEAR,
            )
            GL.glTexParameteri(
                GL.GL_TEXTURE_2D,
                GL.GL_TEXTURE_MAG_FILTER,
                GL.GL_LINEAR,
            )
            GL.glTexParameteri(
                GL.GL_TEXTURE_2D,
                GL.GL_TEXTURE_WRAP_S,
                GL.GL_CLAMP_TO_EDGE,
            )
            GL.glTexParameteri(
                GL.GL_TEXTURE_2D,
                GL.GL_TEXTURE_WRAP_T,
                GL.GL_CLAMP_TO_EDGE,
            )
            GL.glTexImage2D(
                GL.GL_TEXTURE_2D,
                0,
                GL.GL_RGBA8,
                width,
                height,
                0,
                GL.GL_RGBA,
                GL.GL_UNSIGNED_BYTE,
                None,
            )
            GL.glBindTexture(GL.GL_TEXTURE_2D, 0)
        except BaseException:
            surface.close()
            context.clear_current()
            raise
        context.clear_current()
        return cls(context, surface, texture, width, height, scale_factor)

    def descriptor(self) -> render.OpenGLBorrowedTextureDescriptor:
        return render.OpenGLBorrowedTextureDescriptor(
            extent=render.RenderTargetExtent(
                self.width,
                self.height,
                self.scale_factor,
            ),
            context=self.context.descriptor(),
            texture=self.texture,
            target=GL.GL_TEXTURE_2D,
        )

    def exists(self) -> bool:
        self.context.make_current(self.surface)
        try:
            return bool(GL.glIsTexture(self.texture))
        finally:
            self.context.clear_current()

    def close(self) -> None:
        if self._closed:
            return
        self.context.make_current(self.surface)
        try:
            GL.glDeleteTextures(1, [self.texture])
        finally:
            self.context.clear_current()
            self.surface.close()
            self._closed = True

    def __enter__(self) -> "EglBorrowedTexture":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()


@dataclass(slots=True)
class EglPbufferSurface:
    context: EglContext
    surface: Any
    width: int
    height: int
    scale_factor: float
    _closed: bool = False

    @classmethod
    def create(
        cls,
        context: EglContext,
        width: int,
        height: int,
        scale_factor: float,
    ) -> "EglPbufferSurface":
        surface = EGL.eglCreatePbufferSurface(
            context.display,
            context.config,
            [
                EGL.EGL_WIDTH,
                width,
                EGL.EGL_HEIGHT,
                height,
                EGL.EGL_NONE,
            ],
        )
        if _addr(surface) == 0:
            msg = "eglCreatePbufferSurface failed"
            raise EglUnavailableError(msg)
        return cls(context, surface, width, height, scale_factor)

    def descriptor(self) -> render.OpenGLSurfaceDescriptor:
        return render.OpenGLSurfaceDescriptor(
            extent=render.RenderTargetExtent(
                self.width,
                self.height,
                self.scale_factor,
            ),
            context=self.context.descriptor(),
            surface=_pointer(self.surface, "EGLSurface"),
        )

    def close(self) -> None:
        if self._closed:
            return
        EGL.eglDestroySurface(self.context.display, self.surface)
        self._closed = True

    def __enter__(self) -> "EglPbufferSurface":
        return self

    def __exit__(self, *args: object) -> None:
        self.close()
