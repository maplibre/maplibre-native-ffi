"""Offscreen sessions, and borrowed-texture sessions on every backend this
build carries.

GPU objects come from tests/graphics. A backend the build supports whose
graphics object cannot be created fails its tests rather than skipping them.
"""

from __future__ import annotations

import contextlib
import ctypes
import subprocess
import sys
import threading
import time
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
    result,
)

WIDTH, HEIGHT = 32, 16
EXTENT = mln.LogicalExtent(WIDTH, HEIGHT, 1.0)
# Releases a frame whose reads have finished.
DONE = mln.GpuSync(kind=mln.GpuSyncKind.CPU_COMPLETE)


def _backends() -> list[str]:
    mask = mln.supported_render_backend_mask()
    backends = []
    if mask & mln.RenderBackendFlag.METAL:
        backends.append("metal")
    if mask & mln.RenderBackendFlag.VULKAN:
        backends.append("vulkan")
    if mask & mln.RenderBackendFlag.OPEN_GL:
        providers = mln.open_gl_supported_context_provider_mask()
        if providers & mln.OpenGlContextProviderFlag.EGL:
            backends.append("egl")
        if providers & mln.OpenGlContextProviderFlag.WGL:
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
        platform: object = mln.OpenGlContextDescriptorEglVariant(
            mln.EglContextDescriptor(
                display=context.egl_display or 0,
                config=context.egl_config or 0,
                share_context=context.egl_context or 0,
                client_api=mln.OpenGlClientApi.GLES,
                get_proc_address=context.get_proc_address or 0,
            )
        )
    else:
        platform = mln.OpenGlContextDescriptorWglVariant(
            mln.WglContextDescriptor(
                device_context=context.wgl_device_context or 0,
                share_context=context.wgl_context or 0,
                get_proc_address=context.get_proc_address or 0,
            )
        )
    return mln.OpenGlContextDescriptor(
        ownership=mln.OpenGlContextOwnership.SHARED, data=platform
    )


class _Target:
    """Frame demands on a map's render session, whose frame wake notifies
    ``frames``.
    """

    def __init__(self) -> None:
        self.frames = Signal()
        self._results: list[mln.RenderFrameResult] = []
        self._next_token = 1
        self.map: mln.MapHandle | None = None
        self.session: mln.RenderSessionHandle | None = None

    def render(self, *, wait_for_update: bool = False) -> mln.RenderFrameResult:
        """Render one frame, whether or not the map has a newer update."""
        assert self.session is not None
        session = self.session
        token = self._next_token
        self._next_token += 1
        session.request_frame(
            mln.FrameDemand(
                flags=mln.FrameDemandFlag.WAIT_FOR_UPDATE
                if wait_for_update
                else mln.FrameDemandFlag.FORCE,
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

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self,
        kind: type[BaseException] | None,
        value: BaseException | None,
        trace: TracebackType | None,
    ) -> None:
        self.close()

    def close(self) -> None:
        raise NotImplementedError


class Offscreen(_Target):
    """A map rendering offscreen on a device that its session creates.

    ``frame_wake`` makes the frame wake from the function that notifies the
    frame wait.
    """

    def __init__(
        self,
        harness: Harness,
        frame_wake: Callable[[Callable[[], None]], mln.Wake | mln.NativeWake] = (
            mln.Wake
        ),
    ) -> None:
        super().__init__()
        self.map = harness.create_map(
            map_mode=mln.MapMode.CONTINUOUS, initial_extent=EXTENT
        )
        try:
            self.session, attached = self._attach(
                mln.RenderSessionAttachOptions(
                    driver=mln.RenderDriverKind.CORE_WORKER,
                    frame_wake=frame_wake(self.frames.notify),
                )
            )
            result(attached)
        except BaseException:
            with contextlib.suppress(Exception):
                self.close()
            raise

    def _attach(self, options: mln.RenderSessionAttachOptions) -> tuple:
        assert self.map is not None
        return self.map.attach_offscreen(
            mln.OffscreenDescriptor(extent=EXTENT), options
        )

    def render_red(self) -> mln.TextureReadbackResult:
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

    def close(self) -> None:
        # A core-worker session detaches as it closes.
        if self.session is not None:
            result(self.session.close())
            self.session = None
        if self.map is not None:
            close_if_live(self.map)


class BorrowedTexture(_Target):
    """A map rendering into a host texture.

    A caller-driven session is serviced on a Python thread that blocks on the
    session's driver-work wake, and every frame wait blocks on its frame wake.
    An OpenGL context belongs to that thread for its whole life, as a host's
    would: the thread creates it, makes it current, creates the textures, and
    destroys them all.
    """

    def __init__(
        self,
        harness: Harness,
        backend: str,
        driver: mln.RenderDriverKind,
        queue_lock: mln.QueueLock | mln.NativeQueueLock | None = None,
    ) -> None:
        opengl = backend in ("egl", "wgl")
        assert driver == mln.RenderDriverKind.CALLER_GRAPHICS_THREAD or not opengl
        super().__init__()
        self.harness = harness
        self.backend = backend
        self._work = threading.Event()
        self._ready = threading.Event()
        self._stopped = threading.Event()
        self._finish = threading.Event()
        self._stopping = False
        self._release = False
        self._closed = False
        self.serviced = 0
        self.service_errors: list[BaseException] = []
        self.graphics: graphics.Graphics | None = None
        self.textures: list[graphics.Texture] = []
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
                self._create_textures()
            self.map = harness.create_map(
                map_mode=mln.MapMode.CONTINUOUS, initial_extent=EXTENT
            )
            options = mln.RenderSessionAttachOptions(
                driver=driver,
                frame_wake=mln.Wake(self.frames.notify),
                driver_work_wake=mln.Wake(self._work.set),
            )
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

    def _create_textures(self) -> None:
        assert self.graphics is not None
        self.textures.append(graphics.Texture(self.graphics, WIDTH, HEIGHT))

    def _close_textures(self) -> None:
        while self.textures:
            self.textures.pop().close()

    def _attach(self, options: mln.RenderSessionAttachOptions) -> tuple:
        assert self.graphics is not None and self.map is not None
        context = self.graphics.context
        infos = [texture.info for texture in self.textures]
        # Every texture of the ring shares the first one's format and size.
        width, height = infos[0].width, infos[0].height
        if self.backend == "metal":
            return self.map.attach_metal_borrowed_texture(
                mln.MetalBorrowedTextureDescriptor(
                    extent=EXTENT,
                    physical_width=width,
                    physical_height=height,
                    textures=[
                        mln.MetalBorrowedTexture(texture=info.metal_texture or 0)
                        for info in infos
                    ],
                ),
                options,
            )
        if self.backend == "vulkan":
            return self.map.attach_vulkan_borrowed_texture(
                mln.VulkanBorrowedTextureDescriptor(
                    extent=EXTENT,
                    physical_width=width,
                    physical_height=height,
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
                    textures=[
                        mln.VulkanBorrowedTexture(
                            image=info.vulkan_image, image_view=info.vulkan_image_view
                        )
                        for info in infos
                    ],
                    format=infos[0].format,
                    final_layout=infos[0].vulkan_final_layout,
                ),
                options,
            )
        return self.map.attach_open_gl_borrowed_texture(
            mln.OpenGlBorrowedTextureDescriptor(
                extent=EXTENT,
                physical_width=width,
                physical_height=height,
                context=_opengl_context(self.backend, context),
                textures=[
                    mln.OpenGlBorrowedTexture(texture=info.opengl_texture)
                    for info in infos
                ],
                target=infos[0].opengl_target,
            ),
            options,
        )

    def _serve(self, opengl: bool) -> None:
        try:
            if opengl:
                self.graphics = graphics.Graphics(_GRAPHICS[self.backend])
                self.graphics.make_current()
                self._create_textures()
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
            # The context is current here, so it and its textures are
            # destroyed here, once the session that shared them is gone.
            self._finish.wait()
            if opengl and self._release and self.graphics is not None:
                self._close_textures()
                self.graphics.close()

    def with_texture[T](
        self, frame: mln.AcquiredFrameHandle, callback: Callable[[object], T]
    ) -> T:
        if self.backend == "metal":
            view = frame.metal_texture()
        elif self.backend == "vulkan":
            view = frame.vulkan_texture()
        else:
            view = frame.open_gl_texture()
        with view as texture:
            return callback(texture)

    def stop_service(self) -> None:
        if self._service is not None:
            self._stopping = True
            self._work.set()
            if not self._stopped.wait(TIMEOUT):
                raise AssertionError("the graphics thread did not stop")

    def _teardown(self, release: bool) -> None:
        """Without ``release``, a session may still share the graphics object,
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
            self._close_textures()
            self.graphics.close()

    def close(self) -> None:
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


def _default_driver(backend: str) -> mln.RenderDriverKind:
    # OpenGL sessions render on the host's graphics thread.
    if backend in ("egl", "wgl"):
        return mln.RenderDriverKind.CALLER_GRAPHICS_THREAD
    return mln.RenderDriverKind.CORE_WORKER


def _texture_name(backend: str, view: object) -> int:
    return view.image if backend == "vulkan" else view.texture  # type: ignore[attr-defined]


def test_a_session_renders_pixels_the_binding_reads_back(harness: Harness) -> None:
    with Offscreen(harness) as target:
        image = target.render_red()

        assert (image.info.width, image.info.height) == (WIDTH, HEIGHT)
        assert image.info.stride >= WIDTH * 4
        assert len(image.data) == image.info.byte_length
        assert image.data[:4] == RED_PIXEL


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_drain_before_any_demand_returns_no_value(
    harness: Harness, backend: str
) -> None:
    with BorrowedTexture(harness, backend, _default_driver(backend)) as target:
        # Native writes a null batch and a null frame, which read as no
        # results and no frame.
        assert target.session.drain_frame_results() == ()
        assert target.session.acquire_frame() is None


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_caller_driven_session_is_serviced_from_a_python_thread(
    harness: Harness, backend: str
) -> None:
    driver = mln.RenderDriverKind.CALLER_GRAPHICS_THREAD
    with BorrowedTexture(harness, backend, driver) as target:
        assert target.session.get_capabilities().driver == driver
        # The main thread blocks on frame wakes while the graphics thread
        # services the work, so neither holds the GIL across native calls.
        assert (
            target.render(wait_for_update=True).disposition == mln.RenderResult.RENDERED
        )
    # A service call publishes its frame before it returns, so
    # the count is read once closing has stopped the graphics thread.
    assert target.serviced > 0


@pytest.mark.parametrize("backend", BACKENDS)
def test_a_frame_view_holds_its_owners_until_its_scope_ends(
    harness: Harness, backend: str
) -> None:
    with BorrowedTexture(harness, backend, _default_driver(backend)) as target:
        assert (
            target.render(wait_for_update=True).disposition == mln.RenderResult.RENDERED
        )
        frame = target.session.acquire_frame()
        assert frame is not None

        def inspect(view: object) -> int:
            for release in (lambda: frame.close(DONE), target.session.abandon):
                with pytest.raises(mln.MaplibreError) as raised:
                    release()
                assert raised.value.status == mln.Status.BUSY
            return _texture_name(backend, view)

        assert target.with_texture(frame, inspect) != 0
        frame.get_result()
        assert not frame.is_closed
        frame.close(DONE)
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
    with BorrowedTexture(harness, backend, _default_driver(backend)) as target:
        assert (
            target.render(wait_for_update=True).disposition == mln.RenderResult.RENDERED
        )
        frame = target.session.acquire_frame()
        assert frame is not None
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
                frame.close(DONE)
        finally:
            release.set()
            borrower.join()

        assert not failures
        frame.close(DONE)
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


def test_a_native_wake_reaches_native_and_the_first_call_consumes_it(
    harness: Harness,
) -> None:
    # An address must fit a pointer.
    with pytest.raises(OverflowError):
        mln.Wake.native(callback=-1)
    frames = _NativeCode(user_data=0x5EED)
    with Offscreen(harness, frame_wake=frames.wake) as target:
        options = mln.RenderSessionAttachOptions(
            driver=mln.RenderDriverKind.CORE_WORKER
        )
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


def test_leaving_an_attached_sessions_scope_disposes_it(harness: Harness) -> None:
    frames = _NativeCode(user_data=0x5C0)
    with Offscreen(harness, frame_wake=frames.wake) as target:
        assert target.session is not None
        with target.session:
            target.render()
            target.session = None
        # Native code releases the wakes once the session retires.
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
        with BorrowedTexture(harness, backend, driver, queue_lock=lock) as target:
            # Rendering a frame submits to the queue, which native code does
            # only while it holds the lock.
            assert (
                target.render(wait_for_update=True).disposition
                == mln.RenderResult.RENDERED
            )
        assert host.released.wait(TIMEOUT)
        assert host.calls
    else:
        # Only a Vulkan target takes a queue lock, so native code reports the
        # lock as unsupported, and the binding releases its context.
        with (
            BorrowedTexture(harness, backend, driver) as target,
            pytest.raises(
                mln.MaplibreError,
                check=lambda error: error.status == mln.Status.UNSUPPORTED,
            ),
        ):
            target._attach(
                mln.RenderSessionAttachOptions(driver=driver, queue_lock=lock)
            )
    assert host.releases == [host.user_data]


_SHUTDOWN_WITH_AN_ATTACHED_SESSION = """
import atexit
import sys

sys.path.insert(0, sys.argv[1])
from support import Harness
from test_render import BorrowedTexture, _default_driver

backend = sys.argv[2]
objects = []
# Registered before the binding's exit hook, so it runs after that hook, as a
# graphics driver's own exit teardown can. The textures and device are
# destroyed while the session is still open, which is safe only once its
# graphics calls ended.
atexit.register(lambda: [graphics.close() for graphics in objects])
target = BorrowedTexture(Harness(), backend, _default_driver(backend))
target.render()
# An OpenGL context belongs to its graphics thread, which keeps it. The hook
# keeps the session's Vulkan objects, which are children of the device, so a
# Vulkan host keeps its device until the process exits.
if target.graphics is not None and backend not in ("egl", "wgl", "vulkan"):
    objects.extend([*target.textures, target.graphics])
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
