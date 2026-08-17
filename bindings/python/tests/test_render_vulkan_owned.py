from __future__ import annotations

import time
from collections.abc import Callable
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
            )
            try:
                session, attach = map_handle.attach_vulkan_owned_texture(
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
        self.runtime.barrier()
        frame = wait_for_vulkan_frame(self, lambda _: True)
        release_frame(frame)


@pytest.fixture
def vulkan_owned_session() -> VulkanOwnedSession:
    fixture = VulkanOwnedSession.create()
    try:
        yield fixture
    finally:
        fixture.close()


def request_still_image(map_handle: mln.MapHandle) -> mln.OperationHandle[None]:
    return map_handle.request_still_image()


def wait_for_texture_info(
    fixture: VulkanOwnedSession,
    *,
    iterations: int = 5000,
) -> render.TextureImageInfo:
    fixture.map.set_style_json(EMPTY_STYLE_JSON.encode())
    render_until_update(fixture.runtime, fixture.session)
    image = finish_render_operation(
        fixture.session,
        fixture.session.read_premultiplied_rgba8(),
        take_result=True,
    )
    return image.info


def wait_for_vulkan_frame(
    fixture: VulkanOwnedSession,
    predicate: Callable[[render.VulkanOwnedTextureFrame], bool],
    *,
    iterations: int = 5000,
) -> render.VulkanOwnedTextureFrameHandle:
    last_frame: render.VulkanOwnedTextureFrame | None = None
    for _ in range(iterations):
        request_and_finish_frame(fixture.session)
        try:
            frame = fixture.session.acquire_vulkan_owned_texture_frame()
        except mln.InvalidStateError:
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
    result = vulkan_owned_session.session.snapshot.latest_result
    assert result == render.RenderResult.RENDERED
    frame = vulkan_owned_session.session.acquire_vulkan_owned_texture_frame()
    assert frame.result.disposition == result
    assert frame.image.address != 0
    assert frame.image_view.address != 0
    assert (
        frame.device.address == vulkan_owned_session.context.descriptor().device.address
    )
    release_frame(frame)
    assert frame.closed

    # A rendered frame carries the map's repaint request with its result. A
    # static empty style settles, so the signal clears within a few frames
    # once nothing asks to draw again.
    for token in range(2, 32):
        settled = request_and_finish_frame(vulkan_owned_session.session, token=token)
        assert settled.disposition == render.RenderResult.RENDERED
        if settled.needs_repaint is False:
            break
        time.sleep(0.01)
    else:
        raise AssertionError("needs_repaint never cleared for a static style")


def test_core_worker_reads_owned_vulkan_texture(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    info = wait_for_texture_info(vulkan_owned_session)
    image = finish_render_operation(
        vulkan_owned_session.session,
        vulkan_owned_session.session.read_premultiplied_rgba8(),
        take_result=True,
    )
    assert image.info == info
    assert len(image.data) == info.byte_length


def test_owned_vulkan_session_reports_core_worker_capabilities(
    vulkan_owned_session: VulkanOwnedSession,
) -> None:
    capabilities = vulkan_owned_session.session.capabilities
    snapshot = vulkan_owned_session.session.snapshot
    assert capabilities.driver == render.RenderDriver.CORE_WORKER
    assert capabilities.texture_ring_depth in (1, 2, 3)
    assert snapshot.driver == render.RenderDriver.CORE_WORKER
