"""Owned-texture sessions on every backend this build carries.

GPU objects come from tests/graphics. A backend the build supports whose
graphics object cannot be created fails its tests rather than skipping them.
"""

from __future__ import annotations

import threading
from collections.abc import Callable
from dataclasses import replace
from types import TracebackType
from typing import Self

import graphics
import maplibre_native_ffi as mln
import pytest
from support import RED_PIXEL, RED_STYLE, TIMEOUT, Harness, Signal, result

WIDTH, HEIGHT = 32, 16


def _backends() -> list[str]:
    mask = mln.supported_render_backend_mask()
    backends = []
    if mask & mln.RenderBackendFlag.METAL:
        backends.append("metal")
    if mask & mln.RenderBackendFlag.VULKAN:
        backends.append("vulkan")
    if mask & mln.RenderBackendFlag.OPENGL:
        providers = mln.opengl_supported_context_provider_mask()
        if providers & mln.OpenglContextProviderFlag.EGL:
            backends.append("egl")
        if providers & mln.OpenglContextProviderFlag.WGL:
            backends.append("wgl")
    return backends


BACKENDS = _backends()
_GRAPHICS = {
    "metal": graphics.METAL,
    "vulkan": graphics.VULKAN,
    "egl": graphics.EGL,
    "wgl": graphics.WGL,
}


def _opengl_context(backend: str, context: graphics.Context) -> object:
    if backend == "egl":
        platform: object = mln.OpenglContextDescriptorEglVariant(
            mln.EglContextDescriptor(
                display=context.egl_display or 0,
                config=context.egl_config or 0,
                share_context=context.egl_context or 0,
                client_api=mln.OpenglClientApi.GLES,
                get_proc_address=context.get_proc_address or 0,
            )
        )
    else:
        platform = mln.OpenglContextDescriptorWglVariant(
            mln.WglContextDescriptor(
                device_context=context.wgl_device_context or 0,
                share_context=context.wgl_context or 0,
                get_proc_address=context.get_proc_address or 0,
            )
        )
    return mln.OpenglContextDescriptor(
        ownership=mln.OpenglContextOwnership.SHARED, data=platform
    )


class OwnedTexture:
    """A map rendering into a session-owned texture ring.

    A caller-driven session is serviced on a Python thread that blocks on the
    session's driver-work wake, and every frame wait blocks on its frame wake.
    """

    def __init__(
        self, harness: Harness, backend: str, driver: mln.RenderDriverKind
    ) -> None:
        self.harness = harness
        self.backend = backend
        self.frames = Signal()
        self._work = threading.Event()
        self._stopping = False
        self._results: list[mln.RenderFrameResult] = []
        self._next_token = 1
        self.serviced = 0
        self.service_errors: list[BaseException] = []
        self.graphics = graphics.Graphics(_GRAPHICS[backend])
        self.map = harness.map_create(
            map_mode=mln.MapMode.CONTINUOUS,
            initial_extent=mln.LogicalExtent(WIDTH, HEIGHT, 1.0),
        )
        options = replace(
            mln.RenderSessionAttachOptions.default(),
            driver=driver,
            frame_wake=mln.Wake(self.frames.notify),
            driver_work_wake=mln.Wake(self._work.set),
        )
        self.session, attached = self._attach(options)
        self._service: threading.Thread | None = None
        if driver == mln.RenderDriverKind.CALLER_GRAPHICS_THREAD:
            self._service = threading.Thread(target=self._serve, name="graphics")
            self._service.start()
        result(attached)

    def _attach(self, options: mln.RenderSessionAttachOptions) -> tuple:
        extent = mln.RenderTargetExtent(WIDTH, HEIGHT, 1.0)
        context = self.graphics.context
        if self.backend == "metal":
            return self.map.metal_owned_texture_attach(
                mln.MetalOwnedTextureDescriptor(
                    extent=extent,
                    context=mln.MetalContextDescriptor(
                        device=context.metal_device or 0
                    ),
                ),
                options,
            )
        if self.backend == "vulkan":
            return self.map.vulkan_owned_texture_attach(
                mln.VulkanOwnedTextureDescriptor(
                    extent=extent,
                    context=mln.VulkanContextDescriptor(
                        instance=context.vulkan_instance or 0,
                        physical_device=context.vulkan_physical_device or 0,
                        device=context.vulkan_device or 0,
                        graphics_queue=context.vulkan_queue or 0,
                        graphics_queue_family_index=context.vulkan_queue_family_index,
                        get_instance_proc_addr=context.vulkan_get_instance_proc_addr
                        or 0,
                        get_device_proc_addr=context.vulkan_get_device_proc_addr or 0,
                    ),
                ),
                options,
            )
        return self.map.opengl_owned_texture_attach(
            mln.OpenglOwnedTextureDescriptor(
                extent=extent, context=_opengl_context(self.backend, context)
            ),
            options,
        )

    def _serve(self) -> None:
        try:
            if self.backend in ("egl", "wgl"):
                self.graphics.make_current()
            while True:
                self._work.clear()
                if self._stopping:
                    return
                self.serviced += self.session.service_driver_work(0)
                self._work.wait()
        except BaseException as error:  # noqa: BLE001 - reported by close()
            self.service_errors.append(error)
        finally:
            # The context is current here, so it is released here.
            if self.backend in ("egl", "wgl"):
                self.graphics.close()

    def with_texture[T](
        self, frame: mln.AcquiredFrameHandle, callback: Callable[[object], T]
    ) -> T:
        if self.backend == "metal":
            return frame.with_metal_texture(callback)
        if self.backend == "vulkan":
            return frame.with_vulkan_texture(callback)
        return frame.with_opengl_texture(callback)

    def render(self) -> mln.RenderFrameResult:
        """Render one frame, whether or not the map has a newer update."""
        token = self._next_token
        self._next_token += 1
        self.session.request_frame(
            replace(
                mln.FrameDemand.default(), flags=mln.FrameDemandFlag(0), token=token
            )
        )

        def check() -> mln.RenderFrameResult | None:
            try:
                with self.session.drain_frame_results() as batch:
                    self._results.extend(batch.get(i) for i in range(batch.count()))
            except mln.NotReadyError:
                pass
            for index, frame in enumerate(self._results):
                if frame.token == token:
                    return self._results.pop(index)
            return None

        return self.frames.wait_until(check, f"frame {token}")

    def render_red(self) -> mln.TextureReadbackResult:
        """Load a red style, then render until a frame reads back red."""
        result(self.map.set_style_json(RED_STYLE))
        while True:
            assert self.render().disposition == mln.RenderResult.RENDERED
            image = result(self.session.texture_read_premultiplied_rgba8())
            if image.data[:4] == RED_PIXEL:
                return image
            self.harness.wait_event(mln.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)

    def close(self) -> None:
        try:
            if not self.session.closed:
                result(self.session.detach())
        finally:
            # The graphics thread stops before the session is destroyed, so
            # no driver call is in flight when it is.
            if self._service is not None:
                self._stopping = True
                self._work.set()
                self._service.join()
            if not self.session.closed:
                self.session.close()
            if not self.map.closed:
                result(self.map.close())
            self.graphics.close()
        assert not self.service_errors

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        kind: type[BaseException] | None,
        value: BaseException | None,
        trace: TracebackType | None,
    ) -> None:
        self.close()


def _default_driver(backend: str) -> mln.RenderDriverKind:
    # OpenGL sessions render on the host's graphics thread.
    if backend in ("egl", "wgl"):
        return mln.RenderDriverKind.CALLER_GRAPHICS_THREAD
    return mln.RenderDriverKind.CORE_WORKER


def _texture_name(backend: str, view: object) -> int:
    return view.image if backend == "vulkan" else view.texture  # type: ignore[attr-defined]


@pytest.mark.parametrize("backend", BACKENDS)
def test_an_owned_texture_renders_pixels_the_binding_reads_back(
    harness: Harness, backend: str
) -> None:
    with OwnedTexture(harness, backend, _default_driver(backend)) as target:
        image = target.render_red()

        assert (image.info.width, image.info.height) == (WIDTH, HEIGHT)
        assert image.info.stride >= WIDTH * 4
        assert len(image.data) == image.info.byte_length
        assert image.data[:4] == RED_PIXEL


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_caller_driven_session_is_serviced_from_a_python_thread(
    harness: Harness, backend: str
) -> None:
    driver = mln.RenderDriverKind.CALLER_GRAPHICS_THREAD
    with OwnedTexture(harness, backend, driver) as target:
        assert target.session.get_capabilities().driver == driver
        # The main thread blocks on frame wakes while the graphics thread
        # services the work, so neither holds the GIL across native calls.
        assert target.render_red().data[:4] == RED_PIXEL
        assert target.serviced > 0


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_frame_view_holds_its_owners_until_its_scope_ends(
    harness: Harness, backend: str
) -> None:
    with OwnedTexture(harness, backend, _default_driver(backend)) as target:
        target.render_red()
        frame = target.session.acquire_frame()

        def inspect(view: object) -> int:
            # The view reserves the frame and each owner above it, so the
            # binding refuses to release them before native sees the call.
            for release in (frame.close, target.session.abandon):
                with pytest.raises(mln.InvalidStateError) as raised:
                    release()
                assert raised.value.native_status_code is None
            return _texture_name(backend, view)

        assert target.with_texture(frame, inspect) != 0
        with frame:
            assert not frame.closed
        assert frame.closed
        with pytest.raises(mln.InvalidStateError):
            target.with_texture(frame, inspect)


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_borrow_on_another_thread_holds_off_close(
    harness: Harness, backend: str
) -> None:
    with OwnedTexture(harness, backend, _default_driver(backend)) as target:
        target.render_red()
        frame = target.session.acquire_frame()
        entered = threading.Event()
        release = threading.Event()
        failures: list[BaseException] = []

        def hold(view: object) -> None:
            entered.set()
            release.wait()

        def borrow() -> None:
            try:
                target.with_texture(frame, hold)
            except BaseException as error:  # noqa: BLE001 - checked below
                failures.append(error)

        borrower = threading.Thread(target=borrow)
        borrower.start()
        try:
            assert entered.wait(TIMEOUT)
            with pytest.raises(mln.InvalidStateError):
                frame.close()
            assert not frame.closed
        finally:
            release.set()
            borrower.join()

        assert not failures
        frame.close()
        assert frame.closed
