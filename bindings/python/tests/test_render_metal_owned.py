from __future__ import annotations

import gc
import time
import weakref
from collections.abc import Callable, Iterator
from contextlib import closing
from dataclasses import dataclass, replace

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api as render
from maplibre_native_ffi.api import CameraOptions
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
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
    from render_backend_helpers.metal import MetalContext, MetalUnavailableError
except (ImportError, OSError, RuntimeError) as error:  # pragma: no cover
    skip_or_fail_fixture_setup(
        f"Metal Python render fixtures are unavailable: {error}",
        "metal",
        allow_module_level=True,
    )


@dataclass(slots=True)
class MetalOwnedSession:
    runtime: mln.RuntimeHandle
    map: mln.MapHandle
    context: MetalContext
    session: render.RenderSessionHandle

    @classmethod
    def create(
        cls,
        *,
        width: int = 32,
        height: int = 16,
        scale_factor: float = 1.0,
        driver: mln.RenderDriverKind = mln.RenderDriverKind.CORE_WORKER,
        ring_depth: int = 1,
    ) -> MetalOwnedSession:
        if not mln.supported_render_backend_mask() & mln.RenderBackendFlag.METAL:
            skip_or_fail_fixture_setup(
                "native library does not support Metal render sessions",
                "metal",
            )
        try:
            context = MetalContext.create()
        except MetalUnavailableError as error:
            skip_or_fail_fixture_setup(
                f"Metal fixture creation is unavailable: {error}",
                "metal",
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
                session, attach = map_handle.metal_owned_texture_attach(
                    context.owned_texture_descriptor(width, height, scale_factor),
                    replace(
                        mln.RenderSessionAttachOptions.default(),
                        driver=driver,
                        requested_texture_ring_depth=ring_depth,
                    ),
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
        frame = wait_for_metal_frame(self, lambda _: True)
        release_frame(frame)


@pytest.fixture
def metal_owned_session() -> Iterator[MetalOwnedSession]:
    fixture = MetalOwnedSession.create()
    try:
        yield fixture
    finally:
        fixture.close()


def wait_for_metal_frame(
    fixture: MetalOwnedSession,
    predicate: Callable[[render.MetalOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.AcquiredFrameHandle:
    last_frame: render.MetalOwnedTextureFrame | None = None
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
        last_frame = frame.with_metal_texture(lambda view: view)
        if predicate(last_frame):
            return frame
        release_frame(frame)
    raise AssertionError(f"matching Metal frame was not observed; last={last_frame!r}")


def test_core_worker_renders_and_releases_owned_metal_frame(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.map.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(metal_owned_session.runtime, metal_owned_session.session)
    result = metal_owned_session.session.get_snapshot().latest_result
    assert result == render.RenderResult.RENDERED
    frame = metal_owned_session.session.acquire_frame()
    assert frame.get_result().disposition == result
    assert frame.with_metal_texture(lambda view: view.texture) != 0
    assert (
        frame.with_metal_texture(lambda view: view.device)
        == metal_owned_session.context.descriptor().device
    )
    release_frame(frame)
    assert frame.closed

    # A rendered frame carries the map's repaint request with its result. A
    # static empty style settles, so the signal clears within a few frames
    # once nothing asks to draw again.
    for token in range(2, 200):
        settled = request_and_finish_frame(metal_owned_session.session, token=token)
        assert settled.token == token
        if (
            settled.disposition == render.RenderResult.RENDERED
            and settled.needs_repaint is False
        ):
            break
        time.sleep(0.01)
    else:
        raise AssertionError("needs_repaint never cleared for a static style")


def test_core_worker_reads_owned_metal_texture(
    metal_owned_session: MetalOwnedSession,
) -> None:
    info = read_texture_info(
        metal_owned_session.runtime,
        metal_owned_session.map,
        metal_owned_session.session,
    )
    # Readback metadata describes the attached extent and a row-padded buffer.
    assert info.width == 32
    assert info.height == 16
    assert info.stride >= info.width * 4
    assert info.byte_length >= info.stride * info.height

    image = finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.texture_read_premultiplied_rgba8(),
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_metal_session_reports_core_worker_capabilities(
    metal_owned_session: MetalOwnedSession,
) -> None:
    capabilities = metal_owned_session.session.get_capabilities()
    snapshot = metal_owned_session.session.get_snapshot()
    assert capabilities.driver == render.RenderDriverKind.CORE_WORKER
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriverKind.CORE_WORKER


def test_attach_returns_public_render_session_and_rejects_second_session(
    metal_owned_session: MetalOwnedSession,
) -> None:
    session = metal_owned_session.session
    assert isinstance(session, render.RenderSessionHandle)
    assert not session.closed

    # A map drives at most one session, so a second attach is rejected and
    # leaves the first one usable.
    assert_invalid_state(
        lambda: metal_owned_session.map.metal_owned_texture_attach(
            metal_owned_session.context.owned_texture_descriptor(32, 16, 1.0),
            replace(
                mln.RenderSessionAttachOptions.default(),
                driver=mln.RenderDriverKind.CORE_WORKER,
            ),
        )
    )
    assert not session.closed


def test_detached_session_leaves_the_map_free_to_close(
    metal_owned_session: MetalOwnedSession,
) -> None:
    session = metal_owned_session.session
    assert_invalid_state(metal_owned_session.map.close)

    close_session(session)
    assert session.closed
    metal_owned_session.map.close()


def test_frame_demand_without_a_newer_update_reports_no_update(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.render_once()

    # Draining the settled style leaves nothing newer, so a render-if-needed
    # demand terminates without drawing and keeps the session live.
    for token in range(2, 64):
        result = request_and_finish_frame(metal_owned_session.session, token=token)
        if result.disposition == render.RenderResult.NO_UPDATE:
            break
        time.sleep(0.01)
    else:
        raise AssertionError("a settled style never reported NO_UPDATE")

    assert result.needs_repaint is False
    assert not metal_owned_session.session.closed


def test_resize_updates_owned_metal_texture_frame_extent(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.render_once()

    finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.resize(render.RenderTargetExtent(16, 8, 1.0)),
    )
    # The session-owned texture is sized in device pixels, which at the
    # session's fixed scale factor of 1 is the logical extent.
    frame = wait_for_metal_frame(
        metal_owned_session,
        lambda info: info.width == 16,
    )
    try:
        info = frame.with_metal_texture(lambda view: view)
        assert info.height == 8
        assert info.scale_factor == pytest.approx(1.0)
        assert info.generation >= 1
    finally:
        release_frame(frame)


def test_map_size_follows_attach_and_session_resize(
    metal_owned_session: MetalOwnedSession,
) -> None:
    # Attachment sizes the map from the target rather than from map creation.
    assert map_extent(metal_owned_session.map) == (32, 16, pytest.approx(1.0))

    # An applied resize updates the map viewport.
    finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.resize(render.RenderTargetExtent(48, 24, 1.0)),
    )
    assert map_extent(metal_owned_session.map) == (48, 24, pytest.approx(1.0))

    # The scale factor is fixed at attachment, so a session resize that changes
    # it is rejected before any command is submitted.
    with pytest.raises(mln.InvalidArgumentError):
        metal_owned_session.session.resize(render.RenderTargetExtent(48, 24, 2.0))
    assert map_extent(metal_owned_session.map) == (48, 24, pytest.approx(1.0))


def test_metal_frame_view_reserves_frame_and_parent_owners(metal_owned_session):
    frame = wait_for_metal_frame(metal_owned_session, lambda _: True)

    def inspect(view):
        assert view.texture != 0
        assert view.device == metal_owned_session.context.descriptor().device
        assert (view.width, view.height) == (32, 16)
        assert_invalid_state(frame.close)
        assert_invalid_state(metal_owned_session.session.close)
        assert_invalid_state(metal_owned_session.session.abandon)
        assert_invalid_state(metal_owned_session.map.close)

    frame.with_metal_texture(inspect)
    release_frame(frame)
    assert_invalid_state(lambda: frame.with_metal_texture(inspect))


def test_stale_metal_frame_cannot_open_a_view_after_slot_reuse(metal_owned_session):
    stale = wait_for_metal_frame(metal_owned_session, lambda _: True)
    release_frame(stale)
    with wait_for_metal_frame(metal_owned_session, lambda _: True) as current:
        assert current.with_metal_texture(lambda view: view.texture) != 0
        assert_invalid_state(
            lambda: stale.with_metal_texture(lambda view: view.texture)
        )


def test_sibling_frame_finalization_preserves_an_active_view():
    with (
        closing(
            MetalOwnedSession.create(
                ring_depth=2, driver=mln.RenderDriverKind.CALLER_GRAPHICS_THREAD
            )
        ) as fixture,
        wait_for_metal_frame(fixture, lambda _: True) as frame,
    ):
        request_and_finish_frame(
            fixture.session, token=2, flags=render.FrameDemandFlag(0)
        )
        siblings = [fixture.session.acquire_frame()]
        retired = weakref.ref(siblings[0])

        def inspect(view):
            siblings.clear()
            gc.collect()
            assert retired() is None
            assert view.texture != 0
            with pytest.raises(mln.TargetLostError):
                frame.with_metal_texture(lambda _: None)

        frame.with_metal_texture(inspect)
        frame.close()
        fixture.session.close()


def test_session_close_is_rejected_while_a_frame_lease_is_held(
    metal_owned_session: MetalOwnedSession,
) -> None:
    session = metal_owned_session.session
    frame = wait_for_metal_frame(metal_owned_session, lambda _: True)
    try:
        assert session.get_snapshot().acquired_frame_count == 1
        # The host still holds a ring slot, so the session cannot retire.
        assert_invalid_state(session.close)
        assert not session.closed
    finally:
        release_frame(frame)

    assert session.get_snapshot().acquired_frame_count == 0


def test_metal_frame_release_rejection_preserves_ownership(metal_owned_session):
    frame = wait_for_metal_frame(metal_owned_session, lambda _: True)
    with pytest.raises(mln.UnsupportedFeatureError):
        frame.close(replace(mln.GpuSync.default(), kind=mln.GpuSyncKind(999)))
    assert not frame.closed
    assert frame.with_metal_texture(lambda view: view.texture) != 0
    frame.close()
    assert frame.closed


def test_cluster_feature_extension_queries_resolve_unsigned_cluster_id_and_limit(
    metal_owned_session: MetalOwnedSession,
) -> None:
    assert_cluster_feature_extensions(
        metal_owned_session.runtime,
        metal_owned_session.map,
        metal_owned_session.session,
    )


def test_typed_geojson_source_options_cluster_nearby_points(
    metal_owned_session: MetalOwnedSession,
) -> None:
    assert_geojson_cluster_source(
        metal_owned_session.runtime,
        metal_owned_session.map,
        metal_owned_session.session,
    )


def test_metal_frame_demands_report_their_own_tokens(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.render_once()
    assert_frame_demands_report_their_own_tokens(metal_owned_session.session)


def test_metal_texture_ring_exhaustion_reports_not_ready(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.render_once()
    assert_texture_ring_exhaustion_reports_not_ready(
        metal_owned_session.session,
        metal_owned_session.session.acquire_frame,
    )


def test_metal_session_maintenance_commands_round_trip(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.render_once()
    assert_session_maintenance_commands_round_trip(metal_owned_session.session)


def test_metal_frame_lease_releases_itself_when_its_scope_ends(
    metal_owned_session: MetalOwnedSession,
) -> None:
    lease = wait_for_metal_frame(metal_owned_session, lambda _: True)
    with lease:
        assert not lease.closed
        assert metal_owned_session.session.get_snapshot().acquired_frame_count == 1
    assert lease.closed
    assert metal_owned_session.session.get_snapshot().acquired_frame_count == 0


def test_metal_owned_session_rejects_a_borrowed_texture_target(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.render_once()

    # A session-owned ring cannot be handed a caller-owned texture, and the
    # retarget kind is checked before any host handle is read.
    with pytest.raises(mln.UnsupportedFeatureError) as raised:
        metal_owned_session.session.metal_borrowed_texture_set_target(
            replace(
                render.MetalBorrowedTextureDescriptor.default(),
                extent=render.RenderTargetExtent(32, 16, 1.0),
                physical_width=32,
                physical_height=16,
            )
        )
    assert raised.value.status == mln.Status.UNSUPPORTED

    # The rejection left the session rendering.
    assert (
        request_and_finish_frame(
            metal_owned_session.session, token=7001, flags=render.FrameDemandFlag(0)
        ).disposition
        == render.RenderResult.RENDERED
    )


def test_metal_abandon_retires_the_session_and_its_map():
    fixture = MetalOwnedSession.create(
        driver=mln.RenderDriverKind.CALLER_GRAPHICS_THREAD
    )
    try:
        fixture.render_once()
        assert_abandon_retires_the_session(fixture.session, fixture.map)
    finally:
        fixture.close()


def test_rendered_projection_retains_the_rendered_camera(
    metal_owned_session: MetalOwnedSession,
) -> None:
    fixture = metal_owned_session
    with pytest.raises(mln.InvalidStateError):
        fixture.session.projection_create()
    fixture.map.update_camera(
        replace(
            mln.CameraUpdate.default(),
            mode=mln.CameraUpdateMode.JUMP,
            camera=CameraOptions(zoom=3),
            animation=mln.AnimationOptions.default(),
        )
    ).result(timeout=5)
    fixture.render_once()
    fixture.map.update_camera(
        replace(
            mln.CameraUpdate.default(),
            mode=mln.CameraUpdateMode.JUMP,
            camera=CameraOptions(zoom=6),
            animation=mln.AnimationOptions.default(),
        )
    ).result(timeout=5)
    with fixture.session.projection_create() as projection:
        assert projection.get_camera().zoom == 3
        finish_render_operation(
            fixture.session,
            fixture.session.resize(render.RenderTargetExtent(16, 16, 1)),
        )
        with pytest.raises(mln.InvalidStateError):
            fixture.session.projection_create()
        fixture.close()
        assert projection.get_camera().zoom == 3
