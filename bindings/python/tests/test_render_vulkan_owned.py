from __future__ import annotations

import threading
import time
from collections.abc import Callable
from dataclasses import dataclass

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import camera, query, render
from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    assert_cluster_feature_extensions,
    assert_geojson_cluster_source,
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
                mln.MapOptions(width=64, height=64, mode=mln.MapMode.STATIC)
            )
            try:
                session = map_handle.attach_vulkan_owned_texture(
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
        self.map.set_style_json(EMPTY_STYLE_JSON.encode())
        frame = wait_for_vulkan_frame(self, lambda _: True)
        frame.close()


@pytest.fixture
def vulkan_owned_session() -> VulkanOwnedSession:
    fixture = VulkanOwnedSession.create()
    try:
        yield fixture
    finally:
        fixture.close()


def request_still_image_if_needed(map_handle: mln.MapHandle) -> None:
    try:
        map_handle.request_still_image()
    except mln.InvalidStateError as error:
        if "pending still-image request" not in error.diagnostic:
            raise


def wait_for_texture_info(
    fixture: VulkanOwnedSession,
    *,
    iterations: int = 5000,
) -> render.TextureImageInfo:
    fixture.map.set_style_json(EMPTY_STYLE_JSON.encode())
    request_still_image_if_needed(fixture.map)
    for _ in range(iterations):
        fixture.runtime.pump()
        for event in fixture.runtime.drain_events().events:
            if event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE:
                try:
                    fixture.session.render_update()
                except mln.InvalidStateError:
                    pass
        try:
            return fixture.session.texture_image_info()
        except mln.InvalidStateError:
            time.sleep(0.001)
    raise AssertionError("texture readback metadata was not observed")


def wait_for_vulkan_frame(
    fixture: VulkanOwnedSession,
    predicate: Callable[[render.VulkanOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.VulkanOwnedTextureFrameHandle:
    request_still_image_if_needed(fixture.map)
    last_frame: render.VulkanOwnedTextureFrame | None = None
    for _ in range(iterations):
        fixture.runtime.pump()
        for event in fixture.runtime.drain_events().events:
            if event.event_type == mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE:
                try:
                    fixture.session.render_update()
                except mln.InvalidStateError:
                    pass
        try:
            frame = fixture.session.acquire_vulkan_owned_texture_frame()
        except mln.InvalidStateError:
            time.sleep(0.001)
            continue
        last_frame = frame.frame
        if predicate(last_frame):
            return frame
        frame.close()
        time.sleep(0.001)
    raise AssertionError(f"matching Vulkan frame was not observed; last={last_frame!r}")


def assert_invalid_state(call: Callable[[], object]) -> None:
    with pytest.raises(mln.InvalidStateError) as raised:
        call()
    assert raised.value.status == mln.MaplibreStatus.INVALID_STATE


def test_attach_returns_public_render_session_and_rejects_second_session(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    session = vulkan_owned_session.session

    assert isinstance(session, render.RenderSessionHandle)
    assert not session.closed
    assert not session.detached

    with pytest.raises(mln.InvalidStateError) as raised:
        vulkan_owned_session.map.attach_vulkan_owned_texture(
            vulkan_owned_session.context.owned_texture_descriptor(32, 16, 1.0)
        )

    assert raised.value.status == mln.MaplibreStatus.INVALID_STATE
    assert not session.closed


def test_detached_session_leaves_the_map_free_to_close(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    session = vulkan_owned_session.session

    assert_invalid_state(vulkan_owned_session.map.close)

    detached = session.detach()
    vulkan_owned_session.map.close()
    detached.close()

    assert session.closed


def test_render_update_without_pending_update_reports_no_update_and_keeps_session_live(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    update = vulkan_owned_session.session.render_update()
    assert update.result == render.RenderResult.NO_UPDATE
    assert update.needs_repaint is False

    assert not vulkan_owned_session.session.closed
    vulkan_owned_session.session.resize(32, 16, 1.0)


def test_resize_updates_vulkan_owned_texture_frame_extent(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    vulkan_owned_session.session.resize(16, 8, 2.0)
    # The map applies the new logical size on its next pump, and a static map
    # renders only on request, so pump the resize through before requesting the
    # still image. A render before that pump reports the pending size.
    assert (
        vulkan_owned_session.session.render_update().result
        == render.RenderResult.SIZE_PENDING
    )
    vulkan_owned_session.runtime.pump()
    frame = wait_for_vulkan_frame(
        vulkan_owned_session,
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


def test_map_size_follows_attach_and_resize_and_keeps_the_creation_scale_factor(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    # A session enqueues the map size for the map's owner thread rather than
    # setting it in place, so the map keeps its previous size until pumped.
    assert vulkan_owned_session.map.get_size() == (64, 64, pytest.approx(1.0))
    vulkan_owned_session.runtime.pump()
    assert vulkan_owned_session.map.get_size() == (32, 16, pytest.approx(1.0))

    # Resizing at a different scale factor leaves the map's own pixel ratio.
    vulkan_owned_session.session.resize(48, 24, 2.0)
    vulkan_owned_session.runtime.pump()
    assert vulkan_owned_session.map.get_size() == (48, 24, pytest.approx(1.0))


def test_a_worker_thread_attaches_its_own_session_and_renders() -> None:
    """Spec coverage: BND-193, BND-195.

    A session is owned by the thread that attaches it, so a worker thread can
    attach and drive its own session against a map owned by the main thread.
    """
    import threading

    if not mln.supported_render_backends() & mln.RenderBackend.VULKAN:
        pytest.skip("native library does not support Vulkan render sessions")
    try:
        context = VulkanContext.create()
    except VulkanUnavailableError as error:
        pytest.skip(f"Vulkan fixture creation is unavailable: {error}")

    runtime = mln.RuntimeHandle()
    failure: list[BaseException] = []
    rendered: list[bool] = []

    def attach_render_close(map_handle: mln.MapHandle) -> None:
        try:
            session = map_handle.attach_vulkan_owned_texture(
                context.owned_texture_descriptor(32, 16, 1.0)
            )
            try:
                # The map applies its logical size on its own thread, so the
                # first renders report a pending size until the main thread
                # pumps.
                deadline = time.monotonic() + 5.0
                while time.monotonic() < deadline:
                    if session.render_update().result == render.RenderResult.RENDERED:
                        rendered.append(True)
                        break
                    time.sleep(0.002)
            finally:
                # Closing here proves the session is destroyed on the thread
                # that attached it, which is what frees the map to close.
                session.close()
        except BaseException as error:  # noqa: BLE001
            failure.append(error)

    try:
        map_handle = runtime.create_map(mln.MapOptions(width=64, height=64))
        try:
            map_handle.set_style_json(EMPTY_STYLE_JSON.encode())
            worker = threading.Thread(target=attach_render_close, args=(map_handle,))
            worker.start()
            while worker.is_alive():
                # A short park rather than zero: this waits on the worker, so
                # spinning would burn the deadline before it made progress.
                runtime.pump(0.002)
                runtime.drain_events()
            worker.join()
            assert not failure, failure
            assert rendered, "worker thread should render the map"
        finally:
            map_handle.close()
    finally:
        runtime.close()
        context.close()


def test_cpu_readback_metadata_capacity_and_reusable_buffer(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    info = wait_for_texture_info(vulkan_owned_session)

    assert info.width == 32
    assert info.height == 16
    assert info.stride >= info.width * 4
    assert info.byte_length >= info.stride * info.height

    undersized = bytearray([0x7F] * (info.byte_length - 1))
    with pytest.raises(mln.InvalidArgumentError) as raised:
        vulkan_owned_session.session.read_premultiplied_rgba8_into(undersized)
    assert raised.value.status == mln.MaplibreStatus.INVALID_ARGUMENT
    assert set(undersized) == {0x7F}

    # An empty destination is a caller error here, not the C size probe.
    with pytest.raises(mln.InvalidArgumentError):
        vulkan_owned_session.session.read_premultiplied_rgba8_into(bytearray())

    reusable = bytearray(info.byte_length)
    copied = vulkan_owned_session.session.read_premultiplied_rgba8_into(reusable)
    assert copied == info
    assert len(reusable) == info.byte_length


def test_vulkan_frame_acquire_release_and_backend_handles(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    frame = vulkan_owned_session.session.acquire_vulkan_owned_texture_frame()
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
    assert isinstance(image, render.NativePointer)
    assert isinstance(image_view, render.NativePointer)
    assert isinstance(device, render.NativePointer)
    assert image.address != 0
    assert image_view.address != 0
    assert device.address == vulkan_owned_session.context.descriptor().device.address

    frame.close()
    assert frame.closed
    assert_invalid_state(lambda: frame.image)
    assert_invalid_state(lambda: frame.image_view)
    assert_invalid_state(lambda: frame.device)


def test_vulkan_frame_release_failure_leaves_frame_live_for_later_release() -> None:
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
                "image": 0x1000,
                "image_view": 0x2000,
                "device": 0x3000,
                "format": 44,
                "layout": 55,
            }

        def image_address(self) -> int:
            if self.closed:
                raise mln.InvalidStateError(
                    None, "VulkanOwnedTextureFrameHandle is closed"
                )
            return 0x1000

        def image_view_address(self) -> int:
            if self.closed:
                raise mln.InvalidStateError(
                    None, "VulkanOwnedTextureFrameHandle is closed"
                )
            return 0x2000

        def device_address(self) -> int:
            if self.closed:
                raise mln.InvalidStateError(
                    None, "VulkanOwnedTextureFrameHandle is closed"
                )
            return 0x3000

        def close(self) -> None:
            self.close_calls += 1
            if self.close_calls == 1:
                raise mln.InvalidStateError(None, "frame release failed")
            self.closed = True

    native = FakeNativeFrame()
    frame = render.VulkanOwnedTextureFrameHandle._from_native(native)

    assert frame.image.address == 0x1000
    with pytest.raises(mln.InvalidStateError, match="frame release failed"):
        frame.close()

    assert not frame.closed
    assert frame.image.address == 0x1000
    assert native.close_calls == 1

    frame.close()
    assert frame.closed
    assert native.close_calls == 2
    assert_invalid_state(lambda: frame.image)


def test_active_vulkan_frame_rejects_nested_acquire_and_session_operations(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    frame = vulkan_owned_session.session.acquire_vulkan_owned_texture_frame()
    selector = query.FeatureStateSelector(source_id="point", feature_id="feature-1")
    point_query = query.RenderedQueryGeometry.point_geometry(
        camera.ScreenPoint(0.0, 0.0)
    )
    feature = b'{"type":"Feature","geometry":null,"properties":{}}'

    calls: tuple[Callable[[], object], ...] = (
        lambda: vulkan_owned_session.session.resize(16, 16, 1.0),
        vulkan_owned_session.session.render_update,
        vulkan_owned_session.session.detach,
        vulkan_owned_session.session.reduce_memory_use,
        vulkan_owned_session.session.clear_data,
        vulkan_owned_session.session.dump_debug_logs,
        vulkan_owned_session.session.texture_image_info,
        lambda: vulkan_owned_session.session.read_premultiplied_rgba8_into(
            bytearray(4)
        ),
        vulkan_owned_session.session.acquire_metal_owned_texture_frame,
        vulkan_owned_session.session.acquire_vulkan_owned_texture_frame,
        vulkan_owned_session.session.acquire_opengl_owned_texture_frame,
        lambda: vulkan_owned_session.session.query_rendered_features(point_query),
        lambda: vulkan_owned_session.session.query_source_features("point"),
        lambda: vulkan_owned_session.session.query_feature_extensions(
            "point",
            feature,
            "x",
            "y",
        ),
        lambda: vulkan_owned_session.session.set_feature_state(
            selector,
            b'{"hover":true}',
        ),
        lambda: vulkan_owned_session.session.get_feature_state(selector),
        lambda: vulkan_owned_session.session.remove_feature_state(selector),
        vulkan_owned_session.session.close,
    )
    try:
        for call in calls:
            assert_invalid_state(call)
        assert not vulkan_owned_session.session.closed
    finally:
        frame.close()


def test_stale_vulkan_frame_handles_cannot_expose_backend_handles_after_reuse(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    stale_frame = vulkan_owned_session.session.acquire_vulkan_owned_texture_frame()
    stale_image = stale_frame.image
    stale_image_view = stale_frame.image_view
    stale_device = stale_frame.device
    stale_frame.close()

    for pointer in (stale_image, stale_image_view, stale_device):
        assert_invalid_state(lambda pointer=pointer: pointer.address)
    assert_invalid_state(lambda: stale_frame.image)
    assert_invalid_state(lambda: stale_frame.image_view)
    assert_invalid_state(lambda: stale_frame.device)

    next_frame = vulkan_owned_session.session.acquire_vulkan_owned_texture_frame()
    try:
        assert next_frame.image.address != 0
        for pointer in (stale_image, stale_image_view, stale_device):
            assert_invalid_state(lambda pointer=pointer: pointer.address)
    finally:
        next_frame.close()


def test_real_vulkan_render_session_reports_wrong_thread_errors(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    calls: tuple[Callable[[], object], ...] = (
        lambda: vulkan_owned_session.session.resize(16, 16, 1.0),
        vulkan_owned_session.session.render_update,
        vulkan_owned_session.session.acquire_vulkan_owned_texture_frame,
        vulkan_owned_session.session.close,
    )

    def run_call(call: Callable[[], object], observed: list[Exception]) -> None:
        try:
            call()
        except mln.WrongThreadError as error:
            observed.append(error)

    for call in calls:
        observed: list[Exception] = []
        thread = threading.Thread(target=run_call, args=(call, observed))
        thread.start()
        thread.join()

        assert len(observed) == 1
        assert isinstance(observed[0], mln.WrongThreadError)
        assert observed[0].status == mln.MaplibreStatus.WRONG_THREAD
        assert not vulkan_owned_session.session.closed


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
