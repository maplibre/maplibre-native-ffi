from __future__ import annotations

import time
from collections.abc import Callable, Iterator
from dataclasses import dataclass, replace

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import api as render
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
    from render_backend_helpers.vulkan import VulkanContext, VulkanUnavailableError
except (ImportError, OSError, RuntimeError) as error:  # pragma: no cover
    skip_or_fail_fixture_setup(
        f"Vulkan Python render fixtures are unavailable: {error}",
        "vulkan",
        allow_module_level=True,
    )


@dataclass(slots=True)
class VulkanOwnedSession:
    runtime: mln.RuntimeHandle
    map: mln.MapHandle
    context: VulkanContext
    session: render.RenderSessionHandle

    @classmethod
    def create(
        cls,
        *,
        width: int = 32,
        height: int = 16,
        scale_factor: float = 1.0,
        driver: mln.RenderDriverKind = mln.RenderDriverKind.CORE_WORKER,
    ) -> VulkanOwnedSession:
        if not mln.supported_render_backend_mask() & mln.RenderBackendFlag.VULKAN:
            skip_or_fail_fixture_setup(
                "native library does not support Vulkan render sessions",
                "vulkan",
            )
        try:
            context = VulkanContext.create()
        except VulkanUnavailableError as error:
            skip_or_fail_fixture_setup(
                f"Vulkan fixture creation is unavailable: {error}",
                "vulkan",
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
                session, attach = map_handle.vulkan_owned_texture_attach(
                    context.owned_texture_descriptor(width, height, scale_factor),
                    replace(
                        mln.RenderSessionAttachOptions.default(),
                        driver=driver,
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
        frame = wait_for_vulkan_frame(self, lambda _: True)
        release_frame(frame)


@pytest.fixture
def vulkan_owned_session() -> Iterator[VulkanOwnedSession]:
    fixture = VulkanOwnedSession.create()
    try:
        yield fixture
    finally:
        fixture.close()


def wait_for_vulkan_frame(
    fixture: VulkanOwnedSession,
    predicate: Callable[[render.VulkanOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.AcquiredFrameHandle:
    last_frame: render.VulkanOwnedTextureFrame | None = None
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
        last_frame = frame.with_vulkan_texture(lambda view: view)
        if predicate(last_frame):
            return frame
        release_frame(frame)
    raise AssertionError(f"matching Vulkan frame was not observed; last={last_frame!r}")


def test_core_worker_renders_and_releases_owned_vulkan_frame(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.map.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(vulkan_owned_session.runtime, vulkan_owned_session.session)
    result = vulkan_owned_session.session.get_snapshot().latest_result
    assert result == render.RenderResult.RENDERED
    frame = vulkan_owned_session.session.acquire_frame()
    assert frame.get_result().disposition == result
    assert frame.with_vulkan_texture(lambda view: view.image) != 0
    assert frame.with_vulkan_texture(lambda view: view.image_view) != 0
    assert (
        frame.with_vulkan_texture(lambda view: view.device)
        == vulkan_owned_session.context.descriptor().device
    )
    release_frame(frame)
    assert frame.closed

    # A rendered frame carries the map's repaint request with its result. A
    # static empty style settles, so the signal clears within a few frames
    # once nothing asks to draw again.
    for token in range(2, 200):
        settled = request_and_finish_frame(vulkan_owned_session.session, token=token)
        assert settled.token == token
        if (
            settled.disposition == render.RenderResult.RENDERED
            and settled.needs_repaint is False
        ):
            break
        time.sleep(0.01)
    else:
        raise AssertionError("needs_repaint never cleared for a static style")


def test_core_worker_reads_owned_vulkan_texture(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    info = read_texture_info(
        vulkan_owned_session.runtime,
        vulkan_owned_session.map,
        vulkan_owned_session.session,
    )
    # Readback metadata describes the attached extent and a row-padded buffer.
    assert info.width == 32
    assert info.height == 16
    assert info.stride >= info.width * 4
    assert info.byte_length >= info.stride * info.height

    image = finish_render_operation(
        vulkan_owned_session.session,
        vulkan_owned_session.session.texture_read_premultiplied_rgba8(),
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_vulkan_session_reports_core_worker_capabilities(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    capabilities = vulkan_owned_session.session.get_capabilities()
    snapshot = vulkan_owned_session.session.get_snapshot()
    assert capabilities.driver == render.RenderDriverKind.CORE_WORKER
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriverKind.CORE_WORKER


def test_attach_returns_public_render_session_and_rejects_second_session(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    session = vulkan_owned_session.session
    assert isinstance(session, render.RenderSessionHandle)
    assert not session.closed

    # A map drives at most one session, so a second attach is rejected and
    # leaves the first one usable.
    assert_invalid_state(
        lambda: vulkan_owned_session.map.vulkan_owned_texture_attach(
            vulkan_owned_session.context.owned_texture_descriptor(32, 16, 1.0),
            replace(
                mln.RenderSessionAttachOptions.default(),
                driver=mln.RenderDriverKind.CORE_WORKER,
            ),
        )
    )
    assert not session.closed


def test_detached_session_leaves_the_map_free_to_close(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    session = vulkan_owned_session.session
    assert_invalid_state(vulkan_owned_session.map.close)

    close_session(session)
    assert session.closed
    vulkan_owned_session.map.close()


def test_frame_demand_without_a_newer_update_reports_no_update(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    # Draining the settled style leaves nothing newer, so a render-if-needed
    # demand terminates without drawing and keeps the session live.
    for token in range(2, 64):
        result = request_and_finish_frame(vulkan_owned_session.session, token=token)
        if result.disposition == render.RenderResult.NO_UPDATE:
            break
        time.sleep(0.01)
    else:
        raise AssertionError("a settled style never reported NO_UPDATE")

    assert result.needs_repaint is False
    assert not vulkan_owned_session.session.closed


def test_resize_updates_owned_vulkan_texture_frame_extent(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    finish_render_operation(
        vulkan_owned_session.session,
        vulkan_owned_session.session.resize(render.RenderTargetExtent(16, 8, 1.0)),
    )
    # The session-owned texture is sized in device pixels, which at the
    # session's fixed scale factor of 1 is the logical extent.
    frame = wait_for_vulkan_frame(
        vulkan_owned_session,
        lambda info: info.width == 16,
    )
    try:
        info = frame.with_vulkan_texture(lambda view: view)
        assert info.height == 8
        assert info.scale_factor == pytest.approx(1.0)
        assert info.generation >= 1
    finally:
        release_frame(frame)


def test_map_size_follows_attach_and_session_resize(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    # Attachment sizes the map from the target rather than from map creation.
    assert map_extent(vulkan_owned_session.map) == (32, 16, pytest.approx(1.0))

    # An applied resize updates the map viewport.
    finish_render_operation(
        vulkan_owned_session.session,
        vulkan_owned_session.session.resize(render.RenderTargetExtent(48, 24, 1.0)),
    )
    assert map_extent(vulkan_owned_session.map) == (48, 24, pytest.approx(1.0))

    # The scale factor is fixed at attachment, so a session resize that changes
    # it is rejected before any command is submitted.
    with pytest.raises(mln.InvalidArgumentError):
        vulkan_owned_session.session.resize(render.RenderTargetExtent(48, 24, 2.0))
    assert map_extent(vulkan_owned_session.map) == (48, 24, pytest.approx(1.0))


def test_vulkan_view_reserves_its_owner(vulkan_owned_session):
    frame = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)

    def inspect(view):
        assert view.image != 0
        assert_invalid_state(frame.close)
        assert_invalid_state(vulkan_owned_session.session.abandon)

    frame.with_vulkan_texture(inspect)
    frame.close()
    assert_invalid_state(lambda: frame.with_vulkan_texture(inspect))


def test_stale_vulkan_frame_cannot_open_a_view_after_slot_reuse(vulkan_owned_session):
    stale = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    stale.close()
    with wait_for_vulkan_frame(vulkan_owned_session, lambda _: True) as current:
        assert current.with_vulkan_texture(lambda view: view.image) != 0
        assert_invalid_state(lambda: stale.with_vulkan_texture(lambda view: view.image))


def test_session_close_is_rejected_while_a_frame_lease_is_held(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    session = vulkan_owned_session.session
    frame = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    try:
        assert session.get_snapshot().acquired_frame_count == 1
        # The host still holds a ring slot, so the session cannot retire.
        assert_invalid_state(session.close)
        assert not session.closed
    finally:
        release_frame(frame)

    assert session.get_snapshot().acquired_frame_count == 0


def test_vulkan_frame_rejected_release_preserves_ownership(vulkan_owned_session):
    frame = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    with pytest.raises((mln.InvalidArgumentError, mln.UnsupportedFeatureError)):
        frame.close(replace(mln.GpuSync.default(), kind=mln.GpuSyncKind(999)))
    assert not frame.closed
    assert frame.with_vulkan_texture(lambda view: view.image) != 0
    frame.close()
    assert frame.closed


def test_cluster_feature_extension_queries_resolve_unsigned_cluster_id_and_limit(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    assert_cluster_feature_extensions(
        vulkan_owned_session.runtime,
        vulkan_owned_session.map,
        vulkan_owned_session.session,
    )


def test_typed_geojson_source_options_cluster_nearby_points(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    assert_geojson_cluster_source(
        vulkan_owned_session.runtime,
        vulkan_owned_session.map,
        vulkan_owned_session.session,
    )


def test_vulkan_frame_demands_report_their_own_tokens(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()
    assert_frame_demands_report_their_own_tokens(vulkan_owned_session.session)


def test_vulkan_texture_ring_exhaustion_reports_not_ready(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()
    assert_texture_ring_exhaustion_reports_not_ready(
        vulkan_owned_session.session,
        vulkan_owned_session.session.acquire_frame,
    )


def test_vulkan_session_maintenance_commands_round_trip(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()
    assert_session_maintenance_commands_round_trip(vulkan_owned_session.session)


def test_vulkan_frame_lease_releases_itself_when_its_scope_ends(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    lease = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    with lease:
        assert not lease.closed
        assert vulkan_owned_session.session.get_snapshot().acquired_frame_count == 1
    assert lease.closed
    assert vulkan_owned_session.session.get_snapshot().acquired_frame_count == 0


def test_vulkan_owned_session_rejects_a_borrowed_texture_target(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    # A session-owned ring cannot be handed a caller-owned texture, and the
    # retarget kind is checked before any host handle is read.
    with pytest.raises(mln.UnsupportedFeatureError) as raised:
        vulkan_owned_session.session.vulkan_borrowed_texture_set_target(
            replace(
                render.VulkanBorrowedTextureDescriptor.default(),
                extent=render.RenderTargetExtent(32, 16, 1.0),
                physical_width=32,
                physical_height=16,
            )
        )
    assert raised.value.status == mln.Status.UNSUPPORTED

    # The rejection left the session rendering.
    assert (
        request_and_finish_frame(
            vulkan_owned_session.session, token=7001, flags=render.FrameDemandFlag(0)
        ).disposition
        == render.RenderResult.RENDERED
    )


def test_vulkan_abandon_retires_the_session_and_its_map() -> None:
    # The caller driver finishes graphics work before the test abandons the session.
    fixture = VulkanOwnedSession.create(
        driver=mln.RenderDriverKind.CALLER_GRAPHICS_THREAD
    )
    try:
        fixture.render_once()
        assert_abandon_retires_the_session(fixture.session, fixture.map)
    finally:
        fixture.close()
