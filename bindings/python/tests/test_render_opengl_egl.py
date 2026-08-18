from __future__ import annotations

import time
from collections.abc import Callable, Iterator
from contextlib import contextmanager
from dataclasses import dataclass

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import render
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    close_session,
    finish_attach,
    finish_render_operation,
    release_frame,
    render_until_update,
    request_and_finish_frame,
    skip_or_fail_fixture_setup,
)

try:
    from render_backend_helpers.egl import (
        EglBorrowedTexture,
        EglContext,
        EglPbufferSurface,
        EglUnavailableError,
    )
except (
    AttributeError,
    ImportError,
    OSError,
    RuntimeError,
) as error:  # pragma: no cover
    skip_or_fail_fixture_setup(
        f"EGL Python render fixtures are unavailable: {error}",
        "opengl",
        context_provider="egl",
        allow_module_level=True,
    )


@dataclass(slots=True)
class OpenGLOwnedSession:
    runtime: mln.RuntimeHandle
    map: mln.MapHandle
    context: EglContext
    session: render.RenderSessionHandle

    @classmethod
    def create(
        cls,
        *,
        width: int = 32,
        height: int = 16,
        scale_factor: float = 1.0,
    ) -> OpenGLOwnedSession:
        _require_native_opengl_egl_support()
        try:
            context = EglContext.create()
        except EglUnavailableError as error:
            skip_or_fail_fixture_setup(
                f"EGL fixture creation is unavailable: {error}",
                "opengl",
                context_provider="egl",
            )

        runtime = mln.RuntimeHandle()
        try:
            map_handle = runtime.create_map(
                mln.MapOptions(
                    width=width,
                    height=height,
                    scale_factor=scale_factor,
                    mode=mln.MapMode.CONTINUOUS,
                )
            ).result(timeout=5)
            try:
                session, attach = map_handle.attach_opengl_owned_texture(
                    context.owned_texture_descriptor(width, height, scale_factor)
                )
                finish_attach(session, attach)
            except BaseException:
                map_handle.close()
                raise
        except BaseException:
            runtime.close()
            context.close()
            raise

        return cls(runtime, map_handle, context, session)

    def close(self) -> None:
        if not self.session.closed:
            close_session(self.session)
        if not self.map.closed:
            self.map.close()
        if not self.runtime.closed:
            self.runtime.close()
        self.context.close()

    def render_once(self) -> None:
        self.map.set_style_json(EMPTY_STYLE_JSON.encode())
        frame = wait_for_opengl_frame(self, lambda _: True)
        release_frame(frame)


@pytest.fixture
def opengl_owned_session() -> Iterator[OpenGLOwnedSession]:
    fixture = OpenGLOwnedSession.create()
    try:
        yield fixture
    finally:
        fixture.close()


def _require_native_opengl_egl_support() -> None:
    if not (mln.supported_render_backends() & mln.RenderBackend.OPENGL):
        skip_or_fail_fixture_setup(
            "native library does not support OpenGL render sessions",
            "opengl",
            context_provider="egl",
        )
    if not (mln.supported_opengl_context_providers() & mln.OpenGLContextProvider.EGL):
        skip_or_fail_fixture_setup(
            "native library does not support EGL OpenGL contexts",
            "opengl",
            context_provider="egl",
        )


@contextmanager
def _egl_context() -> Iterator[EglContext]:
    _require_native_opengl_egl_support()
    try:
        context = EglContext.create()
    except EglUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"EGL fixture creation is unavailable: {error}",
            "opengl",
            context_provider="egl",
        )

    try:
        yield context
    finally:
        context.close()


@contextmanager
def _egl_pbuffer_surface(context: EglContext) -> Iterator[EglPbufferSurface]:
    try:
        surface = context.pbuffer_surface(width=32, height=16)
    except EglUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"EGL pbuffer fixture creation is unavailable: {error}",
            "opengl",
            context_provider="egl",
        )

    try:
        yield surface
    finally:
        surface.close()


@contextmanager
def _egl_borrowed_texture(
    context: EglContext,
    *,
    width: int = 32,
    height: int = 16,
) -> Iterator[EglBorrowedTexture]:
    try:
        texture = context.borrowed_texture(width=width, height=height)
    except EglUnavailableError as error:
        skip_or_fail_fixture_setup(
            f"EGL texture fixture creation is unavailable: {error}",
            "opengl",
            context_provider="egl",
        )

    try:
        yield texture
    finally:
        texture.close()


def request_still_image(map_handle: mln.MapHandle):
    return map_handle.request_still_image()


def wait_for_texture_info(
    fixture: OpenGLOwnedSession,
    *,
    iterations: int = 5000,
) -> render.TextureImageInfo:
    fixture.map.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(fixture.runtime, fixture.session)
    image = finish_render_operation(
        fixture.session,
        fixture.session.read_premultiplied_rgba8(),
        return_result=True,
    )
    return image.info


def wait_for_opengl_frame(
    fixture: OpenGLOwnedSession,
    predicate: Callable[[render.OpenGLOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.OpenGLOwnedTextureFrameHandle:
    last_frame: render.OpenGLOwnedTextureFrame | None = None
    for _ in range(iterations):
        request_and_finish_frame(fixture.session)
        try:
            frame = fixture.session.acquire_opengl_owned_texture_frame()
        except mln.InvalidStateError:
            time.sleep(0.001)
            continue
        last_frame = frame.frame
        if predicate(last_frame):
            return frame
        release_frame(frame)
    raise AssertionError(f"matching OpenGL frame was not observed; last={last_frame!r}")


def test_caller_driver_renders_and_releases_owned_opengl_frame(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.map.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(opengl_owned_session.runtime, opengl_owned_session.session)
    result = opengl_owned_session.session.snapshot.latest_result
    assert result == render.RenderResult.RENDERED
    frame = opengl_owned_session.session.acquire_opengl_owned_texture_frame()
    assert frame.result.disposition == result
    assert frame.texture.value != 0
    release_frame(frame)
    assert frame.closed


def test_caller_driver_reads_owned_opengl_texture(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    info = wait_for_texture_info(opengl_owned_session)
    image = finish_render_operation(
        opengl_owned_session.session,
        opengl_owned_session.session.read_premultiplied_rgba8(),
        return_result=True,
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_opengl_session_is_always_caller_driven(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    capabilities = opengl_owned_session.session.capabilities
    snapshot = opengl_owned_session.session.snapshot
    assert capabilities.driver == render.RenderDriver.CALLER_GRAPHICS_THREAD
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriver.CALLER_GRAPHICS_THREAD
