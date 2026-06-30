from __future__ import annotations

from collections.abc import Callable
from dataclasses import dataclass
import threading
import time

import pytest

import maplibre_native as mln
from maplibre_native import camera, geo, json, query, render

from render_backend_helpers.runtime import (
    EMPTY_STYLE_JSON,
    skip_or_fail_fixture_setup,
)

try:
    from render_backend_helpers.vulkan import VulkanContext, VulkanUnavailableError
except Exception as error:  # pragma: no cover - host fixture dependency
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
        self.map.set_style_json(EMPTY_STYLE_JSON)
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
        fixture.runtime.run_once()
        while event := fixture.runtime.poll_event():
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


def test_render_update_without_pending_update_keeps_session_live(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    assert_invalid_state(vulkan_owned_session.session.render_update)

    assert not vulkan_owned_session.session.closed
    vulkan_owned_session.session.resize(32, 16, 1.0)


def test_resize_updates_vulkan_owned_texture_frame_extent(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    vulkan_owned_session.render_once()

    vulkan_owned_session.session.resize(16, 8, 2.0)
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
    feature = geo.Feature(geometry=geo.EmptyGeometry())

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
            json.JsonObject((json.JsonMember("hover", True),)),
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
        assert not vulkan_owned_session.session.closed
