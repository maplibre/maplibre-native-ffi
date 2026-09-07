from __future__ import annotations

import time
from collections.abc import Callable, Iterator
from dataclasses import dataclass

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import render
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
    ) -> VulkanOwnedSession:
        if not mln.supported_render_backends() & mln.RenderBackend.VULKAN:
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
                session, attach = map_handle.attach_vulkan_owned_texture(
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
) -> render.VulkanOwnedTextureFrameHandle:
    last_frame: render.VulkanOwnedTextureFrame | None = None
    for _ in range(iterations):
        # Forced rather than render-if-needed: a settled style would otherwise
        # report NO_UPDATE forever and never fill a ring slot.
        request_and_finish_frame(fixture.session, flags=render.FrameDemandFlag(0))
        try:
            frame = fixture.session.acquire_vulkan_owned_texture_frame()
        except mln.InvalidStateError, mln.NotReadyError:
            # No slot holds a frame this host has not already taken.
            time.sleep(0.001)
            continue
        last_frame = frame.frame
        if predicate(last_frame):
            return frame
        release_frame(frame)
    raise AssertionError(f"matching Vulkan frame was not observed; last={last_frame!r}")


def test_core_worker_renders_and_releases_owned_vulkan_frame(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.map.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(vulkan_owned_session.runtime, vulkan_owned_session.session)
    result = vulkan_owned_session.session.snapshot().latest_result
    assert result == render.RenderResult.RENDERED
    frame = vulkan_owned_session.session.acquire_vulkan_owned_texture_frame()
    assert frame.result.disposition == result
    assert frame.image.bits != 0
    assert frame.image_view.bits != 0
    assert (
        frame.device.address == vulkan_owned_session.context.descriptor().device.address
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
        vulkan_owned_session.session.read_premultiplied_rgba8(),
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_vulkan_session_reports_core_worker_capabilities(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    capabilities = vulkan_owned_session.session.capabilities()
    snapshot = vulkan_owned_session.session.snapshot()
    assert capabilities.driver == render.RenderDriver.CORE_WORKER
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriver.CORE_WORKER


def test_attach_returns_public_render_session_and_rejects_second_session(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    session = vulkan_owned_session.session
    assert isinstance(session, render.RenderSessionHandle)
    assert not session.closed

    # A map drives at most one session, so a second attach is rejected and
    # leaves the first one usable.
    assert_invalid_state(
        lambda: vulkan_owned_session.map.attach_vulkan_owned_texture(
            vulkan_owned_session.context.owned_texture_descriptor(32, 16, 1.0)
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
        info = frame.frame
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


def test_vulkan_frame_exposes_backend_handles_only_while_the_lease_is_live(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    frame = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    assert isinstance(frame, render.VulkanOwnedTextureFrameHandle)
    info = frame.frame
    assert info.width == 32
    assert info.height == 16
    assert info.scale_factor == pytest.approx(1.0)
    assert info.generation >= 1
    assert info.frame_id >= 0
    assert info.format != 0

    image = frame.image
    image_view = frame.image_view
    device = frame.device
    assert isinstance(image, render.VulkanHandle)
    assert isinstance(image_view, render.VulkanHandle)
    assert isinstance(device, render.NativePointer)
    assert image.bits != 0
    assert image_view.bits != 0
    assert device.address == vulkan_owned_session.context.descriptor().device.address

    release_frame(frame)
    assert frame.closed
    assert_invalid_state(lambda: frame.image)
    assert_invalid_state(lambda: frame.image_view)
    assert_invalid_state(lambda: frame.device)


def test_stale_vulkan_frame_handles_cannot_expose_backend_handles_after_reuse(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    stale_frame = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    stale_image = stale_frame.image
    stale_image_view = stale_frame.image_view
    stale_device = stale_frame.device
    release_frame(stale_frame)

    for handle in (stale_image, stale_image_view):
        assert_invalid_state(lambda handle=handle: handle.bits)
    assert_invalid_state(lambda: stale_device.address)

    # The ring may hand the same backend object to the next lease, so the
    # retired handles must stay unreadable rather than alias it.
    next_frame = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    try:
        assert next_frame.image.bits != 0
        for handle in (stale_image, stale_image_view):
            assert_invalid_state(lambda handle=handle: handle.bits)
        assert_invalid_state(lambda: stale_device.address)
    finally:
        release_frame(next_frame)


def test_session_close_is_rejected_while_a_frame_lease_is_held(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    session = vulkan_owned_session.session
    frame = wait_for_vulkan_frame(vulkan_owned_session, lambda _: True)
    try:
        assert session.snapshot().acquired_frame_count == 1
        # The host still holds a ring slot, so the session cannot retire.
        assert_invalid_state(session.close)
        assert not session.closed
    finally:
        release_frame(frame)

    assert session.snapshot().acquired_frame_count == 0


def test_vulkan_frame_release_failure_leaves_the_lease_live_for_a_later_release() -> (
    None
):
    class FakeNativeFrame:
        closed = False
        release_calls = 0

        def image_bits(self) -> int:
            if self.closed:
                raise mln.InvalidStateError("VulkanOwnedTextureFrameHandle is closed")
            return 0x1000

        def release(self, kind: int, address: int, value: int) -> None:
            self.release_calls += 1
            if self.release_calls == 1:
                raise mln.InvalidStateError("frame release failed")
            self.closed = True

    native = FakeNativeFrame()
    frame = render.VulkanOwnedTextureFrameHandle._from_native(native)

    assert frame.image.bits == 0x1000
    with pytest.raises(mln.InvalidStateError, match="frame release failed"):
        release_frame(frame)

    # A rejected release keeps the host's claim on the slot, so the handle
    # stays readable and a later release still retires it.
    assert not frame.closed
    assert frame.image.bits == 0x1000
    assert native.release_calls == 1

    release_frame(frame)
    assert frame.closed
    assert native.release_calls == 2
    assert_invalid_state(lambda: frame.image)


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
        vulkan_owned_session.session.acquire_vulkan_owned_texture_frame,
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
        assert vulkan_owned_session.session.snapshot().acquired_frame_count == 1
    assert lease.closed
    assert vulkan_owned_session.session.snapshot().acquired_frame_count == 0


def test_vulkan_owned_session_rejects_a_borrowed_texture_target(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    # A session-owned ring cannot be handed a caller-owned texture, and the
    # retarget kind is checked before any host handle is read.
    with pytest.raises(mln.UnsupportedFeatureError) as raised:
        vulkan_owned_session.session.set_vulkan_borrowed_texture_target(
            render.VulkanBorrowedTextureDescriptor(
                extent=render.RenderTargetExtent(32, 16, 1.0),
                physical_width=32,
                physical_height=16,
            )
        )
    assert raised.value.status == mln.MaplibreStatus.UNSUPPORTED

    # The rejection left the session rendering.
    assert (
        request_and_finish_frame(
            vulkan_owned_session.session, token=7001, flags=render.FrameDemandFlag(0)
        ).disposition
        == render.RenderResult.RENDERED
    )


def test_vulkan_abandon_retires_the_session_and_its_map(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()
    assert_abandon_retires_the_session(
        vulkan_owned_session.session, vulkan_owned_session.map
    )
