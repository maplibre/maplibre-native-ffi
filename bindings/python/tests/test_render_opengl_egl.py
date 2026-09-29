from __future__ import annotations

import time
from collections.abc import Callable, Iterator
from contextlib import contextmanager
from dataclasses import dataclass, replace

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api as render
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
        EGLBorrowedTexture,
        EGLContext,
        EGLPbufferSurface,
        EGLUnavailableError,
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
    context: EGLContext
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
            context = EGLContext.create()
        except EGLUnavailableError as error:
            skip_or_fail_fixture_setup(
                f"EGL fixture creation is unavailable: {error}",
                "opengl",
                context_provider="egl",
            )

        runtime = mln.runtime_create()
        try:
            map_handle = runtime.map_create(
                replace(
                    mln.MapOptions.default(),
                    map_mode=mln.MapMode.CONTINUOUS,
                    initial_extent=mln.LogicalExtent(width, height, scale_factor),
                )
            ).result(timeout=5)
            try:
                session, attach = map_handle.opengl_owned_texture_attach(
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
        self.map.set_style_json(EMPTY_STYLE_JSON.encode()).result(timeout=5)
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
    if not (mln.supported_render_backend_mask() & mln.RenderBackendFlag.OPENGL):
        skip_or_fail_fixture_setup(
            "native library does not support OpenGL render sessions",
            "opengl",
            context_provider="egl",
        )
    if not (
        mln.opengl_supported_context_provider_mask() & mln.OpenglContextProviderFlag.EGL
    ):
        skip_or_fail_fixture_setup(
            "native library does not support EGL OpenGL contexts",
            "opengl",
            context_provider="egl",
        )


@contextmanager
def _egl_context() -> Iterator[EGLContext]:
    _require_native_opengl_egl_support()
    try:
        context = EGLContext.create()
    except EGLUnavailableError as error:
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
def _egl_pbuffer_surface(context: EGLContext) -> Iterator[EGLPbufferSurface]:
    try:
        surface = context.pbuffer_surface(width=32, height=16)
    except EGLUnavailableError as error:
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
    context: EGLContext,
    *,
    width: int = 32,
    height: int = 16,
) -> Iterator[EGLBorrowedTexture]:
    try:
        texture = context.borrowed_texture(width=width, height=height)
    except EGLUnavailableError as error:
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
    predicate: Callable[[render.OpenglOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.AcquiredFrameHandle:
    last_frame: render.OpenglOwnedTextureFrame | None = None
    for _ in range(iterations):
        # Forced rather than render-if-needed: a settled style would otherwise
        # report NO_UPDATE forever and never fill a ring slot.
        request_and_finish_frame(fixture.session, flags=render.FrameDemandFlag(0))
        try:
            frame = fixture.session.acquire_frame()
        except mln.InvalidStateError, mln.NotReadyError:
            # No slot holds a frame this host has not already taken.
            time.sleep(0.001)
            continue
        last_frame = frame.with_opengl_texture(lambda view: view)
        if predicate(last_frame):
            return frame
        release_frame(frame)
    raise AssertionError(f"matching OpenGL frame was not observed; last={last_frame!r}")


def test_caller_driver_renders_and_releases_owned_opengl_frame(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.map.set_style_json(EMPTY_STYLE_JSON.encode()).result(timeout=5)
    render_until_update(opengl_owned_session.runtime, opengl_owned_session.session)
    result = opengl_owned_session.session.get_snapshot().latest_result
    assert result == render.RenderResult.RENDERED
    frame = opengl_owned_session.session.acquire_frame()
    assert frame.get_result().disposition == result
    assert frame.with_opengl_texture(lambda view: view.texture) != 0
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
        opengl_owned_session.session.texture_read_premultiplied_rgba8(),
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_opengl_session_is_always_caller_driven(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    capabilities = opengl_owned_session.session.get_capabilities()
    snapshot = opengl_owned_session.session.get_snapshot()
    assert capabilities.driver == render.RenderDriverKind.CALLER_GRAPHICS_THREAD
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriverKind.CALLER_GRAPHICS_THREAD


def test_attach_returns_public_render_session_and_rejects_second_session(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    session = opengl_owned_session.session
    assert isinstance(session, render.RenderSessionHandle)
    assert not session.closed

    # A map drives at most one session, so a second attach is rejected and
    # leaves the first one usable.
    assert_invalid_state(
        lambda: opengl_owned_session.map.opengl_owned_texture_attach(
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
        info = frame.with_opengl_texture(lambda view: view)
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


def test_opengl_view_reserves_its_owner(opengl_owned_session):
    frame = wait_for_opengl_frame(opengl_owned_session, lambda _: True)

    def inspect(view):
        assert view.texture != 0
        assert_invalid_state(frame.close)
        assert_invalid_state(opengl_owned_session.session.abandon)

    frame.with_opengl_texture(inspect)
    frame.close()
    assert_invalid_state(lambda: frame.with_opengl_texture(inspect))


def test_stale_opengl_frame_cannot_open_a_view_after_slot_reuse(opengl_owned_session):
    stale = wait_for_opengl_frame(opengl_owned_session, lambda _: True)
    stale.close()
    with wait_for_opengl_frame(opengl_owned_session, lambda _: True) as current:
        assert current.with_opengl_texture(lambda view: view.texture) != 0
        assert_invalid_state(
            lambda: stale.with_opengl_texture(lambda view: view.texture)
        )


def test_session_close_is_rejected_while_a_frame_lease_is_held(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    session = opengl_owned_session.session
    frame = wait_for_opengl_frame(opengl_owned_session, lambda _: True)
    try:
        assert session.get_snapshot().acquired_frame_count == 1
        # The host still holds a ring slot, so the session cannot retire.
        assert_invalid_state(session.close)
        assert not session.closed
    finally:
        release_frame(frame)

    assert session.get_snapshot().acquired_frame_count == 0


def test_invalid_opengl_surface_attach_reports_native_status() -> None:
    with mln.runtime_create() as runtime:
        map_handle = runtime.map_create().result(timeout=5)
        try:
            with pytest.raises(
                (mln.InvalidArgumentError, mln.UnsupportedFeatureError)
            ) as raised:
                map_handle.opengl_surface_attach(
                    render.OpenglSurfaceDescriptor.default()
                )
            assert raised.value.status in {
                mln.Status.INVALID_ARGUMENT,
                mln.Status.UNSUPPORTED,
            }
        finally:
            map_handle.close()


def test_egl_pbuffer_surface_session_attaches_and_renders() -> None:
    with (
        _egl_context() as context,
        _egl_pbuffer_surface(context) as surface,
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(),
                initial_extent=mln.LogicalExtent(surface.width, surface.height, 1.0),
            )
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.opengl_surface_attach(surface.descriptor())
        finish_render_operation(session, attach)
        try:
            assert not session.closed
            assert session.get_capabilities().driver == (
                render.RenderDriverKind.CALLER_GRAPHICS_THREAD
            )
            map_handle.set_style_json(EMPTY_STYLE_JSON.encode()).result(timeout=5)
            render_until_update(runtime, session)
        finally:
            close_session(session)


def test_dedicated_egl_surface_renders_and_keeps_its_context_current() -> None:
    with (
        _egl_context() as context,
        _egl_pbuffer_surface(context) as surface,
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(),
                initial_extent=mln.LogicalExtent(surface.width, surface.height, 1.0),
            )
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.opengl_surface_attach(
            surface.dedicated_descriptor()
        )
        finish_render_operation(session, attach)
        try:
            map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode()).result(
                timeout=5
            )
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
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(), initial_extent=mln.LogicalExtent(32, 16, 1.0)
            )
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.opengl_borrowed_texture_attach(
            texture.descriptor()
        )
        finish_render_operation(session, attach)
        try:
            map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode()).result(
                timeout=5
            )
            render_until_update(runtime, session)
        finally:
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
        mln.runtime_create() as runtime,
        runtime.map_create(
            replace(
                mln.MapOptions.default(), initial_extent=mln.LogicalExtent(32, 16, 1.0)
            )
        ).result(timeout=5) as map_handle,
    ):
        session, attach = map_handle.opengl_borrowed_texture_attach(first.descriptor())
        finish_render_operation(session, attach)
        try:
            map_handle.set_style_json(RED_BACKGROUND_STYLE_JSON.encode()).result(
                timeout=5
            )
            render_until_update(runtime, session)

            finish_render_operation(
                session,
                session.opengl_borrowed_texture_set_target(second.descriptor()),
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
        opengl_owned_session.session.acquire_frame,
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
        assert opengl_owned_session.session.get_snapshot().acquired_frame_count == 1
    assert lease.closed
    assert opengl_owned_session.session.get_snapshot().acquired_frame_count == 0


def test_opengl_owned_session_rejects_a_borrowed_texture_target(
    opengl_owned_session: OpenGLOwnedSession,
) -> None:
    opengl_owned_session.render_once()

    # A session-owned ring cannot be handed a caller-owned texture, and the
    # retarget kind is checked before any host handle is read.
    with pytest.raises(mln.UnsupportedFeatureError) as raised:
        opengl_owned_session.session.opengl_borrowed_texture_set_target(
            replace(
                render.OpenglBorrowedTextureDescriptor.default(),
                extent=render.RenderTargetExtent(32, 16, 1.0),
                physical_width=32,
                physical_height=16,
            )
        )
    assert raised.value.status == mln.Status.UNSUPPORTED

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
