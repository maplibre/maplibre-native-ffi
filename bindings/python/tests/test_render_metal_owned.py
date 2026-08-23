from __future__ import annotations

import time
from collections.abc import Callable
from dataclasses import dataclass

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import render
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    assert_cluster_feature_extensions,
    assert_geojson_cluster_source,
    close_session,
    finish_attach,
    finish_render_operation,
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
    ) -> MetalOwnedSession:
        if not mln.supported_render_backends() & mln.RenderBackend.METAL:
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
                session, attach = map_handle.attach_metal_owned_texture(
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
        self.runtime.barrier().result(timeout=5)
        frame = wait_for_metal_frame(self, lambda _: True)
        release_frame(frame)


@pytest.fixture
def metal_owned_session() -> MetalOwnedSession:
    fixture = MetalOwnedSession.create()
    try:
        yield fixture
    finally:
        fixture.close()


def assert_invalid_state(call: Callable[[], object]) -> None:
    with pytest.raises(mln.InvalidStateError) as raised:
        call()
    assert raised.value.status == mln.MaplibreStatus.INVALID_STATE


def wait_for_texture_info(
    fixture: MetalOwnedSession,
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


def wait_for_metal_frame(
    fixture: MetalOwnedSession,
    predicate: Callable[[render.MetalOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.MetalOwnedTextureFrameHandle:
    last_frame: render.MetalOwnedTextureFrame | None = None
    for _ in range(iterations):
        # Forced rather than render-if-needed: a settled style would otherwise
        # report NO_UPDATE forever and never fill a ring slot.
        request_and_finish_frame(fixture.session, flags=render.FrameDemandFlag(0))
        try:
            frame = fixture.session.acquire_metal_owned_texture_frame()
        except mln.InvalidStateError, mln.NotReadyError:
            # No slot holds a frame this host has not already taken.
            time.sleep(0.001)
            continue
        last_frame = frame.frame
        if predicate(last_frame):
            return frame
        release_frame(frame)
    raise AssertionError(f"matching Metal frame was not observed; last={last_frame!r}")


def test_core_worker_renders_and_releases_owned_metal_frame(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.map.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(metal_owned_session.runtime, metal_owned_session.session)
    result = metal_owned_session.session.snapshot.latest_result
    assert result == render.RenderResult.RENDERED
    frame = metal_owned_session.session.acquire_metal_owned_texture_frame()
    assert frame.result.disposition == result
    assert frame.texture.address != 0
    assert (
        frame.device.address == metal_owned_session.context.descriptor().device.address
    )
    release_frame(frame)
    assert frame.closed

    # A rendered frame carries the map's repaint request with its result. A
    # static empty style settles, so the signal clears within a few frames
    # once nothing asks to draw again.
    for token in range(2, 32):
        settled = request_and_finish_frame(metal_owned_session.session, token=token)
        assert settled.disposition == render.RenderResult.RENDERED
        if settled.needs_repaint is False:
            break
        time.sleep(0.01)
    else:
        raise AssertionError("needs_repaint never cleared for a static style")


def test_core_worker_reads_owned_metal_texture(
    metal_owned_session: MetalOwnedSession,
) -> None:
    info = wait_for_texture_info(metal_owned_session)
    # Readback metadata describes the attached extent and a row-padded buffer.
    assert info.width == 32
    assert info.height == 16
    assert info.stride >= info.width * 4
    assert info.byte_length >= info.stride * info.height

    image = finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.read_premultiplied_rgba8(),
        return_result=True,
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_metal_session_reports_core_worker_capabilities(
    metal_owned_session: MetalOwnedSession,
) -> None:
    capabilities = metal_owned_session.session.capabilities
    snapshot = metal_owned_session.session.snapshot
    assert capabilities.driver == render.RenderDriver.CORE_WORKER
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriver.CORE_WORKER


def test_attach_returns_public_render_session_and_rejects_second_session(
    metal_owned_session: MetalOwnedSession,
) -> None:
    session = metal_owned_session.session
    assert isinstance(session, render.RenderSessionHandle)
    assert not session.closed

    # A map drives at most one session, so a second attach is rejected and
    # leaves the first one usable.
    assert_invalid_state(
        lambda: metal_owned_session.map.attach_metal_owned_texture(
            metal_owned_session.context.owned_texture_descriptor(32, 16, 1.0)
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
    finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.resize(render.RenderTargetExtent(32, 16, 1.0)),
    )


def test_resize_updates_owned_metal_texture_frame_extent(
    metal_owned_session: MetalOwnedSession,
) -> None:
    metal_owned_session.render_once()

    finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.resize(render.RenderTargetExtent(16, 8, 2.0)),
    )
    # The session-owned texture is sized in device pixels, so a 16x8 logical
    # extent at scale factor 2 keeps the 32x16 physical ring.
    frame = wait_for_metal_frame(
        metal_owned_session,
        lambda info: info.scale_factor == pytest.approx(2.0),
    )
    try:
        info = frame.frame
        assert info.width == 32
        assert info.height == 16
        assert info.scale_factor == pytest.approx(2.0)
        assert info.generation >= 1
    finally:
        release_frame(frame)


def test_map_size_follows_attach_and_session_resize(
    metal_owned_session: MetalOwnedSession,
) -> None:
    # Attachment sizes the map from the target rather than from map creation.
    assert metal_owned_session.map.get_size() == (32, 16, pytest.approx(1.0))

    # An applied resize updates the map viewport, scale factor included.
    finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.resize(render.RenderTargetExtent(48, 24, 2.0)),
    )
    assert metal_owned_session.map.get_size() == (48, 24, pytest.approx(2.0))


def test_metal_frame_exposes_backend_handles_only_while_the_lease_is_live(
    metal_owned_session: MetalOwnedSession,
) -> None:
    frame = wait_for_metal_frame(metal_owned_session, lambda _: True)
    assert isinstance(frame, render.MetalOwnedTextureFrameHandle)
    info = frame.frame
    assert info.width == 32
    assert info.height == 16
    assert info.scale_factor == pytest.approx(1.0)
    assert info.generation >= 1
    assert info.frame_id >= 0
    assert info.pixel_format != 0

    texture = frame.texture
    device = frame.device
    assert isinstance(texture, render.NativePointer)
    assert isinstance(device, render.NativePointer)
    assert texture.address != 0
    assert device.address == metal_owned_session.context.descriptor().device.address

    release_frame(frame)
    assert frame.closed
    assert_invalid_state(lambda: frame.texture)
    assert_invalid_state(lambda: frame.device)


def test_stale_metal_frame_handles_cannot_expose_backend_handles_after_reuse(
    metal_owned_session: MetalOwnedSession,
) -> None:
    stale_frame = wait_for_metal_frame(metal_owned_session, lambda _: True)
    stale_texture = stale_frame.texture
    stale_device = stale_frame.device
    release_frame(stale_frame)

    for pointer in (stale_texture, stale_device):
        assert_invalid_state(lambda pointer=pointer: pointer.address)

    # The ring may hand the same backend object to the next lease, so the
    # retired pointers must stay unreadable rather than alias it.
    next_frame = wait_for_metal_frame(metal_owned_session, lambda _: True)
    try:
        assert next_frame.texture.address != 0
        for pointer in (stale_texture, stale_device):
            assert_invalid_state(lambda pointer=pointer: pointer.address)
    finally:
        release_frame(next_frame)


def test_session_close_is_rejected_while_a_frame_lease_is_held(
    metal_owned_session: MetalOwnedSession,
) -> None:
    session = metal_owned_session.session
    frame = wait_for_metal_frame(metal_owned_session, lambda _: True)
    try:
        assert session.snapshot.acquired_frame_count == 1
        # The host still holds a ring slot, so the session cannot retire.
        assert_invalid_state(session.close)
        assert not session.closed
    finally:
        release_frame(frame)

    assert session.snapshot.acquired_frame_count == 0


def test_metal_frame_release_failure_leaves_the_lease_live_for_a_later_release() -> (
    None
):
    class FakeNativeFrame:
        closed = False
        release_calls = 0

        def texture_address(self) -> int:
            if self.closed:
                raise mln.InvalidStateError(
                    None, "MetalOwnedTextureFrameHandle is closed"
                )
            return 0x1000

        def release(self, kind: int, address: int, value: int) -> None:
            self.release_calls += 1
            if self.release_calls == 1:
                raise mln.InvalidStateError(None, "frame release failed")
            self.closed = True

    native = FakeNativeFrame()
    frame = render.MetalOwnedTextureFrameHandle._from_native(native)

    assert frame.texture.address == 0x1000
    with pytest.raises(mln.InvalidStateError, match="frame release failed"):
        release_frame(frame)

    # A rejected release keeps the host's claim on the slot, so the address
    # stays readable and a later release still retires it.
    assert not frame.closed
    assert frame.texture.address == 0x1000
    assert native.release_calls == 1

    release_frame(frame)
    assert frame.closed
    assert native.release_calls == 2
    assert_invalid_state(lambda: frame.texture)


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
