"""Owned-texture sessions on every backend this build carries.

GPU objects come from tests/graphics. A backend the build supports whose
graphics object cannot be created fails its tests rather than skipping them.
"""

from __future__ import annotations

import contextlib
import ctypes
import gc
import subprocess
import sys
import threading
import time
import weakref
from collections.abc import Callable
from dataclasses import replace
from pathlib import Path
from types import TracebackType
from typing import Self

import graphics
import maplibre_native_ffi as mln
import pytest
from support import (
    RED_PIXEL,
    RED_STYLE,
    TIMEOUT,
    Harness,
    Signal,
    close_if_live,
    leak_reports,
    result,
)

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
    An OpenGL context belongs to that thread for its whole life, as a host's
    would: the thread creates it, makes it current, and destroys it.
    ``frame_wake`` makes the frame wake from the function that notifies the
    frame wait.
    """

    def __init__(
        self,
        harness: Harness,
        backend: str,
        driver: mln.RenderDriverKind,
        ring_depth: int | None = None,
        frame_wake: Callable[[Callable[[], None]], mln.Wake | mln.NativeWake] = (
            mln.Wake
        ),
        queue_lock: mln.QueueLock | mln.NativeQueueLock | None = None,
    ) -> None:
        opengl = backend in ("egl", "wgl")
        assert driver == mln.RenderDriverKind.CALLER_GRAPHICS_THREAD or not opengl
        self.harness = harness
        self.backend = backend
        self.frames = Signal()
        self._work = threading.Event()
        self._ready = threading.Event()
        self._stopped = threading.Event()
        self._finish = threading.Event()
        self._stopping = False
        self._release = False
        self._closed = False
        self._results: list[mln.RenderFrameResult] = []
        self._next_token = 1
        self.serviced = 0
        self.service_errors: list[BaseException] = []
        self.graphics: graphics.Graphics | None = None
        self.map: mln.MapHandle | None = None
        self.session: mln.RenderSessionHandle | None = None
        self._service: threading.Thread | None = None
        try:
            if driver == mln.RenderDriverKind.CALLER_GRAPHICS_THREAD:
                # A daemon, so a driver call that never returns cannot keep
                # the interpreter from exiting after the test fails.
                self._service = threading.Thread(
                    target=self._serve, args=(opengl,), name="graphics", daemon=True
                )
                self._service.start()
                if not self._ready.wait(TIMEOUT):
                    raise AssertionError("timed out creating the graphics object")
                if self.service_errors:
                    raise self.service_errors[0]
            if not opengl:
                self.graphics = graphics.Graphics(_GRAPHICS[backend])
            self.map = harness.create_map(
                map_mode=mln.MapMode.CONTINUOUS,
                initial_extent=mln.LogicalExtent(WIDTH, HEIGHT, 1.0),
            )
            options = replace(
                mln.RenderSessionAttachOptions.default(),
                driver=driver,
                frame_wake=frame_wake(self.frames.notify),
                driver_work_wake=mln.Wake(self._work.set),
            )
            if ring_depth is not None:
                options = replace(options, requested_texture_ring_depth=ring_depth)
            if queue_lock is not None:
                options = replace(options, queue_lock=queue_lock)
            self.session, attached = self._attach(options)
            # A wake that fired before the session was stored found nothing
            # to service, so the graphics thread checks again.
            self._work.set()
            result(attached)
        except BaseException:
            # A session whose attach failed still detaches before it is
            # destroyed, and close() leaves alive what it cannot detach.
            with contextlib.suppress(Exception):
                self.close()
            raise

    def _attach(self, options: mln.RenderSessionAttachOptions) -> tuple:
        assert self.graphics is not None and self.map is not None
        extent = mln.LogicalExtent(WIDTH, HEIGHT, 1.0)
        context = self.graphics.context
        if self.backend == "metal":
            return self.map.attach_metal_owned_texture(
                mln.MetalOwnedTextureDescriptor(
                    extent=extent,
                    context=mln.MetalContextDescriptor(
                        device=context.metal_device or 0
                    ),
                    # MTLPixelFormatBGRA8Unorm.
                    pixel_format=80,
                ),
                options,
            )
        if self.backend == "vulkan":
            return self.map.attach_vulkan_owned_texture(
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
        return self.map.attach_opengl_owned_texture(
            mln.OpenglOwnedTextureDescriptor(
                extent=extent, context=_opengl_context(self.backend, context)
            ),
            options,
        )

    def _serve(self, opengl: bool) -> None:
        try:
            if opengl:
                self.graphics = graphics.Graphics(_GRAPHICS[self.backend])
                self.graphics.make_current()
        except BaseException as error:  # noqa: BLE001 - raised by __init__
            self.service_errors.append(error)
        finally:
            self._ready.set()
        try:
            while not self.service_errors:
                self._work.wait()
                self._work.clear()
                if self._stopping:
                    break
                if self.session is not None:
                    self.session.service_driver_work()
                    self.serviced += 1
        except BaseException as error:  # noqa: BLE001 - reported by close()
            self.service_errors.append(error)
        finally:
            while not self._stopping:
                self._work.wait()
                self._work.clear()
            self._stopped.set()
            # The context is current here, so it is destroyed here, once the
            # session that shared it is gone.
            self._finish.wait()
            if opengl and self._release and self.graphics is not None:
                self.graphics.close()

    def with_texture[T](
        self, frame: mln.AcquiredFrameHandle, callback: Callable[[object], T]
    ) -> T:
        if self.backend == "metal":
            view = frame.metal_texture()
        elif self.backend == "vulkan":
            view = frame.vulkan_texture()
        else:
            view = frame.opengl_texture()
        with view as texture:
            return callback(texture)

    def render(self, *, wait_for_update: bool = False) -> mln.RenderFrameResult:
        """Render one frame, whether or not the map has a newer update."""
        assert self.session is not None
        session = self.session
        token = self._next_token
        self._next_token += 1
        session.request_frame(
            replace(
                mln.FrameDemand.default(),
                flags=(
                    mln.FrameDemandFlag.IF_NEEDED | mln.FrameDemandFlag.WAIT_FOR_UPDATE
                )
                if wait_for_update
                else mln.FrameDemandFlag(0),
                token=token,
            )
        )

        def check() -> mln.RenderFrameResult | None:
            self._results.extend(session.drain_frame_results())
            for index, frame in enumerate(self._results):
                if frame.token == token:
                    return self._results.pop(index)
            return None

        return self.frames.wait_until(check, f"frame {token}")

    def render_red(self) -> mln.TextureReadbackResult:
        """Load a red style, then render until a frame reads back red."""
        assert self.map is not None and self.session is not None
        result(self.map.set_style_json(RED_STYLE))
        deadline = time.monotonic() + TIMEOUT
        while True:
            assert (
                self.render(wait_for_update=True).disposition
                == mln.RenderResult.RENDERED
            )
            image = result(self.session.read_texture())
            if image.data[:4] == RED_PIXEL:
                return image
            if time.monotonic() > deadline:
                raise AssertionError("timed out waiting for a red frame")

    def stop_service(self) -> None:
        """Stop servicing the session, if the graphics thread services it."""
        if self._service is not None:
            self._stopping = True
            self._work.set()
            if not self._stopped.wait(TIMEOUT):
                raise AssertionError("the graphics thread did not stop")

    def _teardown(self, release: bool) -> None:
        """Stop servicing, destroy the session and map, then the graphics.

        Without ``release``, a session may still share the graphics object,
        so everything is left alive rather than destroyed under it.
        """
        self._closed = True
        self.stop_service()
        # No driver call is in flight once servicing stops, so the session
        # can be destroyed, and it has retired once its close completes.
        if release and self.session is not None:
            result(self.session.close())
        if release and self.map is not None:
            close_if_live(self.map)
        if self._service is not None:
            self._release = release
            self._finish.set()
            self._service.join(TIMEOUT)
            if self._service.is_alive():
                raise AssertionError("the graphics thread did not exit")
        if release and self.graphics is not None:
            self.graphics.close()

    def close(self) -> None:
        """Detach the session, then tear everything down."""
        if self._closed:
            return
        try:
            if self.session is not None:
                result(self.session.detach())
        except BaseException:
            with contextlib.suppress(Exception):
                self._teardown(release=False)
            raise
        self._teardown(release=True)
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
def test_a_drain_before_any_demand_returns_no_value(
    harness: Harness, backend: str
) -> None:
    with OwnedTexture(harness, backend, _default_driver(backend)) as target:
        # Native reports both calls as not ready, which reads as no results and
        # no frame.
        assert target.session.drain_frame_results() == ()
        assert target.session.acquire_frame() is None


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
    # A service call publishes its frame and readback before it returns, so
    # the count is read once closing has stopped the graphics thread.
    assert target.serviced > 0


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_frame_view_holds_its_owners_until_its_scope_ends(
    harness: Harness, backend: str
) -> None:
    with OwnedTexture(harness, backend, _default_driver(backend)) as target:
        target.render_red()
        frame = target.session.acquire_frame()

        def inspect(view: object) -> int:
            for release in (frame.close, target.session.abandon):
                with pytest.raises(mln.MaplibreError) as raised:
                    release()
                assert raised.value.status == mln.Status.BUSY
            return _texture_name(backend, view)

        assert target.with_texture(frame, inspect) != 0
        frame.get_result()
        assert not frame.is_closed
        frame.close()
        # A frame closes once, so a discard after close does nothing.
        frame.discard()
        assert frame.is_closed
        assert repr(frame) == f"AcquiredFrameHandle(id={frame.id:#x}, closed=true)"
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_STATE,
        ):
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
            with pytest.raises(
                mln.MaplibreError, check=lambda error: error.status == mln.Status.BUSY
            ):
                frame.close()
        finally:
            release.set()
            borrower.join()

        assert not failures
        frame.close()
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_STATE,
        ):
            frame.get_result()


@pytest.mark.parametrize("backend", BACKENDS)
def test_finalizing_a_sibling_frame_leaves_an_active_view_intact(
    harness: Harness, backend: str
) -> None:
    driver = _default_driver(backend)
    with OwnedTexture(harness, backend, driver, ring_depth=2) as target:
        target.render_red()
        frame = target.session.acquire_frame()
        target.render()
        siblings = [target.session.acquire_frame()]
        retired = weakref.ref(siblings[0])

        def inspect(view: object) -> int:
            # The collector finalizes the sibling while this frame's view
            # is open, and the view is still the frame's.
            with leak_reports() as reports:
                siblings.clear()
                gc.collect()
            assert retired() is None
            assert reports == ["AcquiredFrameHandle was not explicitly closed"]
            return _texture_name(backend, view)

        assert target.with_texture(frame, inspect) != 0
        frame.close()
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_STATE,
        ):
            frame.get_result()


_NATIVE_CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_void_p)


class _NativeCode:
    """A callback and a release as native function pointers, which stand in
    for a host's native code and record the context that each receives."""

    def __init__(self, user_data: int) -> None:
        self.user_data = user_data
        self.notify: Callable[[], None] = lambda: None
        self.calls: list[int | None] = []
        self.releases: list[int | None] = []
        self.called = threading.Event()
        self.released = threading.Event()
        self.registration: mln.NativeWake | None = None

        def invoke(context: int | None) -> None:
            self.calls.append(context)
            self.called.set()
            self.notify()

        def release(context: int | None) -> None:
            self.releases.append(context)
            self.released.set()

        # The ctypes objects own the function pointers, so they stay alive
        # as long as this object.
        self._callback = _NATIVE_CALLBACK(invoke)
        self._release = _NATIVE_CALLBACK(release)
        self.callback = ctypes.cast(self._callback, ctypes.c_void_p).value or 0
        self.release = ctypes.cast(self._release, ctypes.c_void_p).value or 0

    def wake(self, notify: Callable[[], None]) -> mln.NativeWake:
        self.notify = notify
        self.registration = mln.Wake.native(
            callback=self.callback,
            user_data=self.user_data,
            release_user_data=self.release,
        )
        return self.registration


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_native_wake_reaches_native_and_the_first_call_consumes_it(
    harness: Harness, backend: str
) -> None:
    # An address must fit a pointer.
    with pytest.raises(OverflowError):
        mln.Wake.native(callback=-1)
    frames = _NativeCode(user_data=0x5EED)
    driver = _default_driver(backend)
    with OwnedTexture(harness, backend, driver, frame_wake=frames.wake) as target:
        options = mln.RenderSessionAttachOptions.default()
        target.render()
        # Native code calls the function pointer with the context unchanged.
        assert frames.called.wait(TIMEOUT)
        assert set(frames.calls) == {frames.user_data}

        # The attach consumed it, so another call raises before native code.
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_STATE,
        ):
            target._attach(replace(options, frame_wake=frames.registration))

        # Native code rejects a disabled wake that carries a release, so the
        # binding releases the context before the call returns.
        rejected = _NativeCode(user_data=0xD1E)
        with pytest.raises(
            mln.MaplibreError,
            check=lambda error: error.status == mln.Status.INVALID_ARGUMENT,
        ):
            target._attach(
                replace(
                    options,
                    frame_wake=mln.Wake.native(
                        callback=0,
                        user_data=rejected.user_data,
                        release_user_data=rejected.release,
                    ),
                )
            )
        assert rejected.releases == [rejected.user_data]
        assert frames.releases == []

    # Native code releases the accepted wake once the session is gone.
    assert frames.released.wait(TIMEOUT)
    assert frames.releases == [frames.user_data]


@pytest.mark.parametrize("backend", BACKENDS)
def test_leaving_an_attached_sessions_scope_disposes_it(
    harness: Harness, backend: str
) -> None:
    frames = _NativeCode(user_data=0x5C0)
    driver = _default_driver(backend)
    with OwnedTexture(harness, backend, driver, frame_wake=frames.wake) as target:
        assert target.session is not None
        with target.session:
            target.render()
            # Nothing services the session once its scope disposes it.
            target.stop_service()
            target.session = None
        # Native code releases the wakes once the session retires, after
        # which the map releases and the host destroys the graphics objects.
        assert frames.released.wait(TIMEOUT)


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_native_queue_lock_reaches_native(harness: Harness, backend: str) -> None:
    host = _NativeCode(user_data=0x10C)
    lock = mln.QueueLock.native(
        lock=host.callback,
        unlock=host.callback,
        user_data=host.user_data,
        release_user_data=host.release,
    )
    driver = _default_driver(backend)
    if backend == "vulkan":
        with OwnedTexture(harness, backend, driver, queue_lock=lock) as target:
            # Rendering and reading back a styled frame submits to the queue,
            # which native code does only while it holds the lock.
            target.render_red()
        assert host.released.wait(TIMEOUT)
        assert host.calls
    else:
        # Only a Vulkan target takes a queue lock, so native code reports the
        # lock as unsupported, and the binding releases its context.
        with (
            OwnedTexture(harness, backend, driver) as target,
            pytest.raises(
                mln.MaplibreError,
                check=lambda error: error.status == mln.Status.UNSUPPORTED,
            ),
        ):
            target._attach(
                replace(mln.RenderSessionAttachOptions.default(), queue_lock=lock)
            )
    assert host.releases == [host.user_data]


_SHUTDOWN_WITH_AN_ATTACHED_SESSION = """
import atexit
import sys

sys.path.insert(0, sys.argv[1])
from support import Harness
from test_render import OwnedTexture, _default_driver

backend = sys.argv[2]
devices = []
# Registered before the binding's exit hook, so it runs after that hook, as a
# graphics driver's own exit teardown can. The device is destroyed while the
# session is still open, which is safe only once its graphics calls ended.
atexit.register(lambda: [device.close() for device in devices])
target = OwnedTexture(Harness(), backend, _default_driver(backend))
target.render_red()
# An OpenGL context belongs to its graphics thread, which keeps it. The hook
# keeps the session's Vulkan objects, which are children of the device, so a
# Vulkan host keeps its device until the process exits.
if target.graphics is not None and backend not in ("egl", "wgl", "vulkan"):
    devices.append(target.graphics)
print("RENDERED", flush=True)
"""


@pytest.mark.skipif(
    sys.platform in ("android", "ios"),
    reason="an embedded interpreter has no executable to start",
)
@pytest.mark.parametrize("backend", BACKENDS)
def test_interpreter_shutdown_with_an_attached_session_exits_cleanly(
    backend: str,
) -> None:
    completed = subprocess.run(
        [
            sys.executable,
            "-c",
            _SHUTDOWN_WITH_AN_ATTACHED_SESSION,
            str(Path(__file__).parent),
            backend,
        ],
        check=False,
        capture_output=True,
        text=True,
        timeout=3 * TIMEOUT,
    )

    assert completed.returncode == 0, completed.stderr
    assert "RENDERED" in completed.stdout
    assert "Exception ignored" not in completed.stderr
    assert "Fatal Python error" not in completed.stderr
    assert "panicked" not in completed.stderr
