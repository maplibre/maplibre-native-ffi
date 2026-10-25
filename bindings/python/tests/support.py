"""The shared runtime fixture and the one wait that every test uses."""

from __future__ import annotations

import contextlib
import os
import sys
import threading
import time
import warnings
from collections.abc import Callable, Iterator
from concurrent.futures import Future
from dataclasses import replace

import maplibre_native_ffi as mln

# Emulators, simulators, and software renderers set a larger scale.
TIMEOUT = 10.0 * float(os.environ.get("MLN_TEST_TIMEOUT_SCALE", "1"))

EMPTY_STYLE = b'{"version":8,"sources":{},"layers":[]}'
RED_STYLE = (
    b'{"version":8,"sources":{},"layers":[{"id":"background",'
    b'"type":"background","paint":{"background-color":"#ff0000"}}]}'
)
RED_PIXEL = b"\xff\x00\x00\xff"


def ok_response(data: bytes = EMPTY_STYLE) -> mln.ResourceResponse:
    return mln.ResourceResponse(
        status=mln.ResourceResponseStatus.OK,
        error_reason=mln.ResourceErrorReason.NONE,
        bytes=data,
        error_message="",
        must_revalidate=False,
        etag="",
    )


def result[T](future: Future[T]) -> T:
    return future.result(timeout=TIMEOUT)


def ready_map(
    runtime: mln.RuntimeHandle, options: mln.MapOptions | None = None
) -> mln.MapHandle:
    """Create a map and wait until it is ready."""
    created = runtime.create_map(options)
    result(created.ready)
    return created.map


@contextlib.contextmanager
def leak_reports() -> Iterator[list[str]]:
    """Collect the ResourceWarning messages that unclosed handles report.

    The list fills when the block exits. A report can come from any thread,
    such as a native callback thread that drops the last reference, so this
    relies on the process-wide warning filters that an interpreter without
    context-aware warnings keeps. The abi3 extension does not load on a
    free-threaded build, where those are the default.
    """
    if getattr(sys.flags, "context_aware_warnings", False):
        raise RuntimeError("leak_reports needs process-wide warning filters")
    reports: list[str] = []
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always", ResourceWarning)
        yield reports
    reports.extend(
        str(warning.message)
        for warning in caught
        if issubclass(warning.category, ResourceWarning)
    )


class Signal:
    """A wake that native invokes, and the wait that blocks on it.

    ``wait_until`` checks its condition, then blocks until the next wake or
    the deadline. Clearing before each check keeps a wake that arrives during
    the check.
    """

    def __init__(self) -> None:
        self._event = threading.Event()

    def notify(self) -> None:
        self._event.set()

    def wait_until[T](self, check: Callable[[], T | None], what: str) -> T:
        deadline = time.monotonic() + TIMEOUT
        while True:
            self._event.clear()
            value = check()
            if value is not None:
                return value
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not self._event.wait(remaining):
                raise AssertionError(f"timed out waiting for {what}")


def denied_response() -> mln.ResourceResponse:
    return mln.ResourceResponse(
        status=mln.ResourceResponseStatus.ERROR,
        error_reason=mln.ResourceErrorReason.NOT_FOUND,
        bytes=b"",
        must_revalidate=False,
        error_message="the test provider serves no such resource",
        etag="",
    )


type Route = Callable[[mln.ResourceRequestInfo, mln.ResourceRequestHandle], None]


class Harness:
    """One runtime whose events wake the test, and whose provider denies
    every request that no route serves, so no test reaches the network."""

    def __init__(self) -> None:
        self.events = Signal()
        self.routes: dict[str, Route] = {}
        self._pending: list[mln.RuntimeEvent] = []
        self._maps: list[mln.MapHandle] = []
        self.runtime = mln.runtime_create(mln.RuntimeOptions.default())
        # Both commands run before the runtime creates a map, so its first
        # events and requests reach them without waiting for completion.
        self.runtime.discarding.set_event_handler(
            mln.RuntimeEventHandler(self._receive_events)
        )
        self.runtime.discarding.set_resource_provider(
            mln.ResourceProvider(callback=self._provide)
        )

    def _receive_events(self, events: tuple[mln.RuntimeEvent, ...]) -> None:
        self._pending.extend(events)
        self.events.notify()

    def _provide(
        self, request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        route = self.routes.get(request.requested_url)
        if route is not None:
            route(request, handle)
            return
        handle.complete(denied_response())

    def create_map(self, **options: object) -> mln.MapHandle:
        map_handle = ready_map(
            self.runtime, replace(mln.MapOptions.default(), **options)
        )
        self._maps.append(map_handle)
        return map_handle

    def wait_event(
        self,
        event_type: mln.RuntimeEventType,
        match: Callable[[mln.RuntimeEvent], bool] = lambda _: True,
    ) -> mln.RuntimeEvent:
        """Return the first event of this type that matches, draining as the
        runtime's event handler receives copies."""

        def check() -> mln.RuntimeEvent | None:
            for index, event in enumerate(self._pending):
                if event.type == event_type and match(event):
                    del self._pending[index]
                    return event
            return None

        return self.events.wait_until(check, event_type.name)

    def close(self) -> None:
        for map_handle in self._maps:
            close_if_live(map_handle)
        close_if_live(self.runtime)


def close_if_live(owner: mln.MapHandle | mln.RuntimeHandle) -> None:
    """Close a fixture owner unless native already retired it."""
    try:
        if isinstance(owner, mln.MapHandle):
            owner.get_snapshot()
        else:
            owner.get_event_mask()
    except mln.MaplibreError as error:
        if error.status == mln.Status.INVALID_STATE:
            return
        raise
    result(owner.close())
