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
            )
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
        self.runtime.barrier()
        frame = wait_for_metal_frame(self, lambda _: True)
        release_frame(frame)


@pytest.fixture
def metal_owned_session() -> MetalOwnedSession:
    fixture = MetalOwnedSession.create()
    try:
        yield fixture
    finally:
        fixture.close()


def request_still_image(map_handle: mln.MapHandle) -> mln.OperationHandle[None]:
    return map_handle.request_still_image()


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
        take_result=True,
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
        request_and_finish_frame(fixture.session)
        try:
            frame = fixture.session.acquire_metal_owned_texture_frame()
        except mln.NotReadyError:
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


def test_core_worker_reads_owned_metal_texture(
    metal_owned_session: MetalOwnedSession,
) -> None:
    info = wait_for_texture_info(metal_owned_session)
    image = finish_render_operation(
        metal_owned_session.session,
        metal_owned_session.session.read_premultiplied_rgba8(),
        take_result=True,
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
