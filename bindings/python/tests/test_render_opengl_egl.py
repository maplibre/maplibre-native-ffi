from __future__ import annotations

from collections.abc import Callable, Iterator
from contextlib import contextmanager
from dataclasses import dataclass
import threading
import time

import pytest

import maplibre_native as mln
from maplibre_native import camera, geo, json, query, render

from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    render_until_update,
    skip_or_fail_fixture_setup,
)

try:
    from render_backend_helpers.egl import (
        EglBorrowedTexture,
        EglContext,
        EglPbufferSurface,
        EglUnavailableError,
    )
except Exception as error:  # pragma: no cover - host fixture dependency
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
                mln.MapOptions(width=64, height=64, mode=mln.MapMode.STATIC)
            )
            try:
                session = map_handle.attach_opengl_owned_texture(
                    context.owned_texture_descriptor(width, height, scale_factor)
                )
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
            self.session.close()
        if not self.map.closed:
            self.map.close()
        if not self.runtime.closed:
            self.runtime.close()
        self.context.close()

    def render_once(self) -> None:
        self.map.set_style_json(EMPTY_STYLE_JSON)
        frame = wait_for_opengl_frame(self, lambda _: True)
        frame.close()


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
def _egl_borrowed_texture(context: EglContext) -> Iterator[EglBorrowedTexture]:
    try:
        texture = context.borrowed_texture(width=32, height=16)
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


def request_still_image_if_needed(map_handle: mln.MapHandle) -> None:
    try:
        map_handle.request_still_image()
    except mln.InvalidStateError as error:
        if "pending still-image request" not in error.diagnostic:
            raise


def wait_for_texture_info(
    fixture: OpenGLOwnedSession,
    *,
    iterations: int = 5000,
) -> render.TextureImageInfo:
    fixture.map.set_style_json(EMPTY_STYLE_JSON)
    request_still_image_if_needed(fixture.map)
    for _ in range(iterations):
        fixture.runtime.run_once()
        while event := fixture.runtime.poll_event():
            if event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE:
                try:
                    fixture.session.render_update()
                except mln.InvalidStateError:
                    pass
        try:
            return fixture.session.texture_image_info()
        except mln.InvalidStateError:
            time.sleep(0.001)
    raise AssertionError("OpenGL texture readback metadata was not observed")


def wait_for_opengl_frame(
    fixture: OpenGLOwnedSession,
    predicate: Callable[[render.OpenGLOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.OpenGLOwnedTextureFrameHandle:
    request_still_image_if_needed(fixture.map)
    last_frame: render.OpenGLOwnedTextureFrame | None = None
    for _ in range(iterations):
        fixture.runtime.run_once()
        while event := fixture.runtime.poll_event():
            if event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE:
                try:
                    fixture.session.render_update()
                except mln.InvalidStateError:
                    pass
        try:
            frame = fixture.session.acquire_opengl_owned_texture_frame()
        except mln.InvalidStateError:
            time.sleep(0.001)
            continue
        last_frame = frame.frame
        if predicate(last_frame):
            return frame
        frame.close()
        time.sleep(0.001)
    raise AssertionError(f"matching OpenGL frame was not observed; last={last_frame!r}")


def assert_invalid_state(call: Callable[[], object]) -> None:
    with pytest.raises(mln.InvalidStateError) as raised:
        call()
    assert raised.value.status == mln.MaplibreStatus.INVALID_STATE


def _assert_public_session_shape(session: render.RenderSessionHandle) -> None:
    assert isinstance(session, render.RenderSessionHandle)
    assert session.closed is False
    assert session.detached is False
    assert callable(session.render_update)
    assert callable(session.close)


def _borrowed_descriptor_snapshot(
    descriptor: render.OpenGLBorrowedTextureDescriptor,
) -> tuple[object, ...]:
    context = descriptor.context
    assert isinstance(context, render.EglContextDescriptor)
    return (
        descriptor.extent.width,
        descriptor.extent.height,
        descriptor.extent.scale_factor,
        context.display.address,
        context.config.address,
        context.share_context.address,
        context.get_proc_address.address,
        descriptor.texture,
        descriptor.target,
    )


def test_invalid_opengl_surface_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime:
        with runtime.create_map() as map_handle:
            with pytest.raises(
                (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
            ) as raised:
                map_handle.attach_opengl_surface(render.OpenGLSurfaceDescriptor())

            assert raised.value.status in {
                mln.MaplibreStatus.INVALID_ARGUMENT,
                mln.MaplibreStatus.UNSUPPORTED,
            }


def test_egl_pbuffer_surface_attach_reports_public_session_shape() -> None:
    with _egl_context() as context:
        with _egl_pbuffer_surface(context) as surface:
            with mln.RuntimeHandle() as runtime:
                with runtime.create_map(
                    mln.MapOptions(width=surface.width, height=surface.height)
                ) as map_handle:
                    session = map_handle.attach_opengl_surface(surface.descriptor())
                    try:
                        _assert_public_session_shape(session)

                        map_handle.set_style_json(EMPTY_STYLE_JSON)
                        render_until_update(runtime, session)
                    finally:
                        session.close()


def test_attach_returns_public_render_session_and_rejects_second_session(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    session = opengl_owned_session.session

    _assert_public_session_shape(session)

    with pytest.raises(mln.InvalidStateError) as raised:
        opengl_owned_session.map.attach_opengl_owned_texture(
            opengl_owned_session.context.owned_texture_descriptor(32, 16, 1.0)
        )

    assert raised.value.status == mln.MaplibreStatus.INVALID_STATE
    assert not session.closed


def test_render_update_without_pending_update_keeps_session_live(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    assert_invalid_state(opengl_owned_session.session.render_update)

    assert not opengl_owned_session.session.closed
    opengl_owned_session.session.resize(32, 16, 1.0)


def test_resize_updates_opengl_owned_texture_frame_extent(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    opengl_owned_session.session.resize(16, 8, 2.0)
    frame = wait_for_opengl_frame(
        opengl_owned_session,
        lambda info: (
            info.width == 32 and info.height == 16 and info.scale_factor == 2.0
        ),
    )
    try:
        info = frame.frame
        assert info.width == 32
        assert info.height == 16
        assert info.scale_factor == pytest.approx(2.0)
        assert info.generation >= 2
    finally:
        frame.close()


def test_cpu_readback_metadata_capacity_and_reusable_buffer(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    info = wait_for_texture_info(opengl_owned_session)

    assert info.width == 32
    assert info.height == 16
    assert info.stride >= info.width * 4
    assert info.byte_length >= info.stride * info.height

    undersized = bytearray([0x7F] * (info.byte_length - 1))
    with pytest.raises(mln.InvalidArgumentError) as raised:
        opengl_owned_session.session.read_premultiplied_rgba8_into(undersized)
    assert raised.value.status == mln.MaplibreStatus.INVALID_ARGUMENT
    assert set(undersized) == {0x7F}

    reusable = bytearray(info.byte_length)
    copied = opengl_owned_session.session.read_premultiplied_rgba8_into(reusable)
    assert copied == info
    assert len(reusable) == info.byte_length


def test_opengl_frame_acquire_release_and_backend_texture_scope(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    frame = opengl_owned_session.session.acquire_opengl_owned_texture_frame()
    assert isinstance(frame, render.OpenGLOwnedTextureFrameHandle)
    info = frame.frame
    assert info.width == 32
    assert info.height == 16
    assert info.scale_factor == pytest.approx(1.0)
    assert info.generation >= 1
    assert info.frame_id >= 0
    assert info.target != 0
    assert info.internal_format != 0
    assert info.format != 0
    assert info.type != 0

    texture = frame.texture
    assert isinstance(texture, render.FrameOpenGLTextureName)
    assert texture.value != 0

    frame.close()
    assert frame.closed
    assert_invalid_state(lambda: frame.texture)
    assert_invalid_state(lambda: texture.value)


def test_opengl_frame_release_failure_leaves_frame_live_for_later_release() -> None:
    class FakeNativeFrame:
        closed = False
        close_calls = 0

        def frame(self) -> dict[str, object]:
            return {
                "generation": 1,
                "width": 32,
                "height": 16,
                "scale_factor": 1.0,
                "frame_id": 7,
                "target": 0x0DE1,
                "internal_format": 0x8058,
                "format": 0x1908,
                "type": 0x1401,
            }

        def texture(self) -> int:
            if self.closed:
                raise mln.InvalidStateError(None, "OpenGL frame is closed")
            return 5

        def close(self) -> None:
            self.close_calls += 1
            if self.close_calls == 1:
                raise mln.InvalidStateError(None, "frame release failed")
            self.closed = True

    native = FakeNativeFrame()
    frame = render.OpenGLOwnedTextureFrameHandle._from_native(native)

    assert frame.texture.value == 5
    with pytest.raises(mln.InvalidStateError, match="frame release failed"):
        frame.close()

    assert not frame.closed
    assert frame.texture.value == 5
    assert native.close_calls == 1

    frame.close()
    assert frame.closed
    assert native.close_calls == 2
    assert_invalid_state(lambda: frame.texture)


def test_active_opengl_frame_rejects_nested_acquire_and_session_operations(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    frame = opengl_owned_session.session.acquire_opengl_owned_texture_frame()
    selector = query.FeatureStateSelector(source_id="point", feature_id="feature-1")
    point_query = query.RenderedQueryGeometry.point_geometry(
        camera.ScreenPoint(0.0, 0.0)
    )
    feature = geo.Feature(geometry=geo.EmptyGeometry())

    calls: tuple[Callable[[], object], ...] = (
        lambda: opengl_owned_session.session.resize(16, 16, 1.0),
        opengl_owned_session.session.render_update,
        opengl_owned_session.session.detach,
        opengl_owned_session.session.reduce_memory_use,
        opengl_owned_session.session.clear_data,
        opengl_owned_session.session.dump_debug_logs,
        opengl_owned_session.session.texture_image_info,
        lambda: opengl_owned_session.session.read_premultiplied_rgba8_into(
            bytearray(4)
        ),
        opengl_owned_session.session.acquire_metal_owned_texture_frame,
        opengl_owned_session.session.acquire_vulkan_owned_texture_frame,
        opengl_owned_session.session.acquire_opengl_owned_texture_frame,
        lambda: opengl_owned_session.session.query_rendered_features(point_query),
        lambda: opengl_owned_session.session.query_source_features("point"),
        lambda: opengl_owned_session.session.query_feature_extensions(
            "point",
            feature,
            "x",
            "y",
        ),
        lambda: opengl_owned_session.session.set_feature_state(
            selector,
            json.JsonObject((json.JsonMember("hover", True),)),
        ),
        lambda: opengl_owned_session.session.get_feature_state(selector),
        lambda: opengl_owned_session.session.remove_feature_state(selector),
        opengl_owned_session.session.close,
    )
    try:
        for call in calls:
            assert_invalid_state(call)
        assert not opengl_owned_session.session.closed
    finally:
        frame.close()


def test_stale_opengl_texture_names_cannot_expose_value_after_reuse(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    stale_frame = opengl_owned_session.session.acquire_opengl_owned_texture_frame()
    stale_texture = stale_frame.texture
    stale_frame.close()

    assert_invalid_state(lambda: stale_texture.value)
    assert_invalid_state(lambda: stale_frame.texture)

    next_frame = opengl_owned_session.session.acquire_opengl_owned_texture_frame()
    try:
        assert next_frame.texture.value != 0
        assert_invalid_state(lambda: stale_texture.value)
    finally:
        next_frame.close()


def test_real_opengl_render_session_reports_wrong_thread_errors(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    calls: tuple[Callable[[], object], ...] = (
        lambda: opengl_owned_session.session.resize(16, 16, 1.0),
        opengl_owned_session.session.render_update,
        opengl_owned_session.session.acquire_opengl_owned_texture_frame,
        opengl_owned_session.session.close,
    )

    for call in calls:
        observed: list[BaseException] = []

        def run_call() -> None:
            try:
                call()
            except BaseException as error:
                observed.append(error)

        thread = threading.Thread(target=run_call)
        thread.start()
        thread.join()

        assert len(observed) == 1
        assert isinstance(observed[0], mln.WrongThreadError)
        assert observed[0].status == mln.MaplibreStatus.WRONG_THREAD
        assert not opengl_owned_session.session.closed


def test_egl_borrowed_texture_session_close_preserves_caller_resources() -> None:
    with _egl_context() as context:
        with _egl_borrowed_texture(context) as texture:
            descriptor = texture.descriptor()
            before_descriptor = _borrowed_descriptor_snapshot(descriptor)
            before_texture = texture.texture

            with mln.RuntimeHandle() as runtime:
                with runtime.create_map(
                    mln.MapOptions(
                        width=descriptor.extent.width,
                        height=descriptor.extent.height,
                    )
                ) as map_handle:
                    session = map_handle.attach_opengl_borrowed_texture(descriptor)
                    try:
                        _assert_public_session_shape(session)

                        map_handle.set_style_json(EMPTY_STYLE_JSON)
                        render_until_update(runtime, session)

                        with pytest.raises(mln.UnsupportedFeatureError) as raised:
                            session.acquire_opengl_owned_texture_frame()
                        assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED
                    finally:
                        session.close()

            assert _borrowed_descriptor_snapshot(descriptor) == before_descriptor
            assert texture.texture == before_texture
            assert texture.exists()

            replacement_descriptor = texture.descriptor()
            assert _borrowed_descriptor_snapshot(replacement_descriptor) == (
                before_descriptor
            )
