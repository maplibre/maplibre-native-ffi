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
    RED_BACKGROUND_STYLE_JSON,
    RED_PIXEL,
    assert_abandon_retires_the_session,
    assert_cluster_feature_extensions,
    assert_frame_demands_report_their_own_tokens,
    assert_geojson_cluster_source,
    assert_invalid_state,
    assert_session_maintenance_commands_round_trip,
    assert_texture_ring_exhaustion_reports_not_ready,
    close_session,
    finish_render_operation,
    map_extent,
    read_texture_info,
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
        current_context_address,
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
                finish_render_operation(session, attach)
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


def wait_for_opengl_frame(
    fixture: OpenGLOwnedSession,
    predicate: Callable[[render.OpenGLOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.OpenGLOwnedTextureFrameHandle:
    last_frame: render.OpenGLOwnedTextureFrame | None = None
    for _ in range(iterations):
        # Forced rather than render-if-needed: a settled style would otherwise
        # report NO_UPDATE forever and never fill a ring slot.
        request_and_finish_frame(fixture.session, flags=render.FrameDemandFlag(0))
        try:
            frame = fixture.session.acquire_opengl_owned_texture_frame()
        except mln.InvalidStateError, mln.NotReadyError:
            # No slot holds a frame this host has not already taken.
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
    result = opengl_owned_session.session.snapshot().latest_result
    assert result == render.RenderResult.RENDERED
    frame = opengl_owned_session.session.acquire_opengl_owned_texture_frame()
    assert frame.result.disposition == result
    assert frame.texture.value != 0
    release_frame(frame)
    assert frame.closed


def test_caller_driver_reads_owned_opengl_texture(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    info = read_texture_info(
        opengl_owned_session.runtime,
        opengl_owned_session.map,
        opengl_owned_session.session,
    )
    # Readback metadata describes the attached extent and a row-padded buffer.
    assert info.width == 32
    assert info.height == 16
    assert info.stride >= info.width * 4
    assert info.byte_length >= info.stride * info.height

    image = finish_render_operation(
        opengl_owned_session.session,
        opengl_owned_session.session.read_premultiplied_rgba8(),
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_opengl_session_is_always_caller_driven(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    capabilities = opengl_owned_session.session.capabilities()
    snapshot = opengl_owned_session.session.snapshot()
    assert capabilities.driver == render.RenderDriver.CALLER_GRAPHICS_THREAD
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriver.CALLER_GRAPHICS_THREAD


def test_attach_returns_public_render_session_and_rejects_second_session(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    session = opengl_owned_session.session
    assert isinstance(session, render.RenderSessionHandle)
    assert not session.closed

    # A map drives at most one session, so a second attach is rejected and
    # leaves the first one usable.
    assert_invalid_state(
        lambda: opengl_owned_session.map.attach_opengl_owned_texture(
            opengl_owned_session.context.owned_texture_descriptor(32, 16, 1.0)
        )
    )
    assert not session.closed


def test_detached_session_leaves_the_map_free_to_close(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    session = opengl_owned_session.session
    assert_invalid_state(opengl_owned_session.map.close)

    close_session(session)
    assert session.closed
    opengl_owned_session.map.close()


def test_frame_demand_without_a_newer_update_reports_no_update(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    # Draining the settled style leaves nothing newer, so a render-if-needed
    # demand terminates without drawing and keeps the session live.
    for token in range(2, 64):
        result = request_and_finish_frame(opengl_owned_session.session, token=token)
        if result.disposition == render.RenderResult.NO_UPDATE:
            break
        time.sleep(0.01)
    else:
        raise AssertionError("a settled style never reported NO_UPDATE")

    assert result.needs_repaint is False
    assert not opengl_owned_session.session.closed


def test_resize_updates_owned_opengl_texture_frame_extent(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    finish_render_operation(
        opengl_owned_session.session,
        opengl_owned_session.session.resize(render.RenderTargetExtent(16, 8, 1.0)),
    )
    # The session-owned texture is sized in device pixels, which at the
    # session's fixed scale factor of 1 is the logical extent.
    frame = wait_for_opengl_frame(
        opengl_owned_session,
        lambda info: info.width == 16,
    )
    try:
        info = frame.frame
        assert info.height == 8
        assert info.scale_factor == pytest.approx(1.0)
        assert info.generation >= 1
    finally:
        release_frame(frame)


def test_map_size_follows_attach_and_session_resize(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    # Attachment sizes the map from the target rather than from map creation.
    assert map_extent(opengl_owned_session.map) == (32, 16, pytest.approx(1.0))

    # An applied resize updates the map viewport.
    finish_render_operation(
        opengl_owned_session.session,
        opengl_owned_session.session.resize(render.RenderTargetExtent(48, 24, 1.0)),
    )
    assert map_extent(opengl_owned_session.map) == (48, 24, pytest.approx(1.0))

    # The scale factor is fixed at attachment, so a session resize that changes
    # it is rejected before any command is submitted.
    with pytest.raises(mln.InvalidArgumentError):
        opengl_owned_session.session.resize(render.RenderTargetExtent(48, 24, 2.0))
    assert map_extent(opengl_owned_session.map) == (48, 24, pytest.approx(1.0))


def test_opengl_frame_exposes_its_texture_name_only_while_the_lease_is_live(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    frame = wait_for_opengl_frame(opengl_owned_session, lambda _: True)
    info = frame.frame
    assert info.width == 32
    assert info.height == 16
    assert info.scale_factor == pytest.approx(1.0)
    assert info.generation >= 1
    assert info.frame_id >= 0
    assert info.target != 0
    # The ring reports the texture's own OpenGL formats, which a host needs to
    # sample or blit it.
    assert info.internal_format != 0
    assert info.format != 0
    assert info.type != 0

    texture = frame.texture
    assert isinstance(texture, render.FrameOpenGLTextureName)
    assert texture.value != 0

    release_frame(frame)
    assert frame.closed
    assert_invalid_state(lambda: frame.texture)


def test_stale_opengl_texture_names_cannot_expose_value_after_reuse(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    stale_frame = wait_for_opengl_frame(opengl_owned_session, lambda _: True)
    stale_texture = stale_frame.texture
    release_frame(stale_frame)

    assert_invalid_state(lambda: stale_texture.value)

    # The ring may hand the same texture name to the next lease, so the
    # retired name must stay unreadable rather than alias it.
    next_frame = wait_for_opengl_frame(opengl_owned_session, lambda _: True)
    try:
        assert next_frame.texture.value != 0
        assert_invalid_state(lambda: stale_texture.value)
    finally:
        release_frame(next_frame)


def test_session_close_is_rejected_while_a_frame_lease_is_held(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    session = opengl_owned_session.session
    frame = wait_for_opengl_frame(opengl_owned_session, lambda _: True)
    try:
        assert session.snapshot().acquired_frame_count == 1
        # The host still holds a ring slot, so the session cannot retire.
        assert_invalid_state(session.close)
        assert not session.closed
    finally:
        release_frame(frame)

    assert session.snapshot().acquired_frame_count == 0


def test_invalid_opengl_surface_attach_reports_native_status() -> None:
    with mln.RuntimeHandle() as runtime:
        map_handle = runtime.create_map().result(timeout=5)
        try:
            with pytest.raises(
                (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
            ) as raised:
                map_handle.attach_opengl_surface(render.OpenGLSurfaceDescriptor())
            assert raised.value.status in {
                mln.MaplibreStatus.INVALID_ARGUMENT,
                mln.MaplibreStatus.UNSUPPORTED,
            }
        finally:
            map_handle.close()


def test_egl_pbuffer_surface_session_attaches_and_renders() -> None:
    with (
        _egl_context() as context,
        _egl_pbuffer_surface(context) as surface,
        mln.RuntimeHandle() as runtime,
        runtime.create_map(
            mln.MapOptions(width=surface.width, height=surface.height)
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.attach_opengl_surface(surface.descriptor())
        finish_render_operation(session, attach)
        try:
            assert not session.closed
            assert session.capabilities().driver == (
                render.RenderDriver.CALLER_GRAPHICS_THREAD
            )
            map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
            render_until_update(runtime, session)
        finally:
            close_session(session)


def test_dedicated_egl_surface_renders_and_keeps_its_context_current() -> None:
    with (
        _egl_context() as context,
        _egl_pbuffer_surface(context) as surface,
        mln.RuntimeHandle() as runtime,
        runtime.create_map(
            mln.MapOptions(width=surface.width, height=surface.height)
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.attach_opengl_surface(
            surface.dedicated_descriptor()
        )
        finish_render_operation(session, attach)
        try:
            map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode())
            render_until_update(runtime, session)

            # A dedicated context belongs to the session, so it stays
            # current on this thread between renders rather than being
            # restored to what the session found.
            assert current_context_address() != 0
        finally:
            close_session(session)

        # Detaching hands the thread back with no context current.
        assert current_context_address() == 0


def test_egl_borrowed_texture_session_close_preserves_caller_resources() -> None:
    with (
        _egl_context() as context,
        _egl_borrowed_texture(context) as texture,
        mln.RuntimeHandle() as runtime,
        runtime.create_map(mln.MapOptions(width=32, height=16)).result(
            timeout=5
        ) as map_handle,
    ):
        session, attach = map_handle.attach_opengl_borrowed_texture(
            texture.descriptor()
        )
        finish_render_operation(session, attach)
        map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode())
        render_until_update(runtime, session)
        close_session(session)

        # The texture is caller-owned, so the session leaves it alive for
        # the host to read and destroy.
        assert texture.exists()
        assert texture.read_rgba()[:4] == RED_PIXEL


def test_egl_borrowed_texture_set_target_hands_over_a_replacement() -> None:
    with (
        _egl_context() as context,
        _egl_borrowed_texture(context) as first,
        _egl_borrowed_texture(context) as second,
        mln.RuntimeHandle() as runtime,
        runtime.create_map(mln.MapOptions(width=32, height=16)).result(
            timeout=5
        ) as map_handle,
    ):
        session, attach = map_handle.attach_opengl_borrowed_texture(first.descriptor())
        finish_render_operation(session, attach)
        try:
            map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode())
            render_until_update(runtime, session)

            finish_render_operation(
                session,
                session.set_opengl_borrowed_texture_target(second.descriptor()),
            )
            render_until_update(runtime, session)

            # Rendering moved to the replacement, and the retired texture stays
            # the caller's to destroy.
            assert second.read_rgba()[:4] == RED_PIXEL
            assert first.exists()
        finally:
            close_session(session)


def test_cluster_feature_extension_queries_resolve_unsigned_cluster_id_and_limit(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    assert_cluster_feature_extensions(
        opengl_owned_session.runtime,
        opengl_owned_session.map,
        opengl_owned_session.session,
    )


def test_typed_geojson_source_options_cluster_nearby_points(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    assert_geojson_cluster_source(
        opengl_owned_session.runtime,
        opengl_owned_session.map,
        opengl_owned_session.session,
    )


def test_opengl_frame_demands_report_their_own_tokens(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()
    assert_frame_demands_report_their_own_tokens(opengl_owned_session.session)


def test_opengl_texture_ring_exhaustion_reports_not_ready(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()
    assert_texture_ring_exhaustion_reports_not_ready(
        opengl_owned_session.session,
        opengl_owned_session.session.acquire_opengl_owned_texture_frame,
    )


def test_opengl_session_maintenance_commands_round_trip(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()
    assert_session_maintenance_commands_round_trip(opengl_owned_session.session)


def test_opengl_frame_lease_releases_itself_when_its_scope_ends(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    lease = wait_for_opengl_frame(opengl_owned_session, lambda _: True)
    with lease:
        assert not lease.closed
        assert opengl_owned_session.session.snapshot().acquired_frame_count == 1
    assert lease.closed
    assert opengl_owned_session.session.snapshot().acquired_frame_count == 0


def test_opengl_owned_session_rejects_a_borrowed_texture_target(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    # A session-owned ring cannot be handed a caller-owned texture, and the
    # retarget kind is checked before any host handle is read.
    with pytest.raises(mln.UnsupportedFeatureError) as raised:
        opengl_owned_session.session.set_opengl_borrowed_texture_target(
            render.OpenGLBorrowedTextureDescriptor(
                extent=render.RenderTargetExtent(32, 16, 1.0),
                physical_width=32,
                physical_height=16,
            )
        )
    assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED

    # The rejection left the session rendering.
    assert (
        request_and_finish_frame(
            opengl_owned_session.session, token=7001, flags=render.FrameDemandFlag(0)
        ).disposition
        == render.RenderResult.RENDERED
    )


def test_opengl_abandon_retires_the_session_and_its_map(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()
    assert_abandon_retires_the_session(
        opengl_owned_session.session, opengl_owned_session.map
    )
