"""Python callbacks that native holds and invokes."""

import gc
import sys
import threading
import typing
from concurrent.futures import Future

import maplibre_native_ffi as mln
import pytest
from maplibre_native_ffi import _native
from support import (
    TIMEOUT,
    Harness,
    denied_response,
    ok_response,
    result,
)


def test_a_camera_end_handler_survives_a_collection_and_still_runs(
    map_handle: mln.MapHandle,
) -> None:
    # The handler sits two records deep in the command's input, and nothing
    # else references it once the command is dropped.
    ended = threading.Event()
    update = mln.CameraUpdate(
        mode=mln.CameraUpdateMode.EASE,
        camera=mln.CameraOptions(zoom=4.0),
        animation=mln.AnimationOptions(
            duration_ms=60000.0,
            transition_id=3,
            end_handler=mln.CameraTransitionHandler(lambda _: ended.set()),
        ),
    )
    result(map_handle.update_camera(update))
    del update
    gc.collect()
    # Cancelling the transition runs the handler before the command completes.
    result(map_handle.cancel_camera_transitions_with_id(3))
    assert ended.is_set()


def test_an_exception_in_a_callback_is_reported_and_contained(
    harness: Harness, map_handle: mln.MapHandle, monkeypatch: pytest.MonkeyPatch
) -> None:
    reports: list[object] = []
    monkeypatch.setattr(sys, "unraisablehook", reports.append)

    def fail(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        raise RuntimeError("provider failed")

    harness.routes["custom://raises.json"] = fail
    map_handle.set_style_url("custom://raises.json")
    # Native fails the request when the callback raises.
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)

    (report,) = reports
    assert isinstance(report.exc_value, RuntimeError)
    assert str(report.exc_value) == "provider failed"


def test_a_provider_can_answer_a_request_later_from_another_thread(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()

    def defer(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        handles.append(handle)
        provided.set()

    harness.routes["custom://later.json"] = defer
    map_handle.set_style_url("custom://later.json")
    assert provided.wait(TIMEOUT)
    handle = handles.pop()
    assert handle.is_cancelled() is False

    answer = threading.Thread(target=handle.complete, args=(ok_response(),))
    answer.start()
    answer.join(TIMEOUT)
    assert not answer.is_alive()
    harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)
    # The answer consumed the handle, so a second answer fails.
    assert handle.is_closed
    with pytest.raises(
        mln.MaplibreError, check=lambda error: error.status == mln.Status.INVALID_STATE
    ):
        handle.complete(ok_response())


def test_a_refused_answer_retires_its_request(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    handles: list[mln.ResourceRequestHandle] = []
    provided = threading.Event()

    def defer(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        handles.append(handle)
        provided.set()

    harness.routes["custom://refused.json"] = defer
    map_handle.set_style_url("custom://refused.json")
    assert provided.wait(TIMEOUT)
    handle = handles.pop()
    # The response fails to convert before native sees the answer, which still
    # retires the handle and fails the request.
    with pytest.raises(AttributeError):
        handle.complete(typing.cast("mln.ResourceResponse", "not a response"))
    assert handle.is_closed
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)


def test_a_transform_request_is_answered_from_another_thread_after_its_callback(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    style = "custom://transformed.json"
    received: list[mln.ResourceTransformRequestHandle] = []
    taken = threading.Event()

    def transform(
        kind: mln.ResourceKind, url: str, request: mln.ResourceTransformRequestHandle
    ) -> None:
        if url != style:
            request.release()
            return
        received.append(request)
        taken.set()

    # The provider passes the style through to the online source, which waits
    # for the transform's answer.
    harness.routes[style] = lambda request, handle: handle.pass_through()
    result(
        harness.runtime.set_resource_transform(
            mln.ResourceTransform(callback=transform)
        )
    )
    map_handle.set_style_url(style)
    assert taken.wait(TIMEOUT)
    request = received[0]

    def answer() -> None:
        request.complete("unsupported://rewritten.json")

    worker = threading.Thread(target=answer)
    worker.start()
    worker.join(TIMEOUT)
    # The answer consumed the request, so a second answer fails.
    assert request.is_closed
    with pytest.raises(
        mln.MaplibreError, check=lambda error: error.status == mln.Status.INVALID_STATE
    ):
        request.complete("unsupported://again.json")
    # The online source cannot load the rewritten scheme.
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)
    result(harness.runtime.clear_resource_transform())


def test_a_log_callback_survives_a_collection_and_still_runs(
    map_handle: mln.MapHandle,
) -> None:
    logged = threading.Event()
    try:
        mln.set_log_callback(mln.LogHandler(lambda *_: logged.set()))
        gc.collect()
        map_handle.set_style_json([])
        assert logged.wait(TIMEOUT)
    finally:
        mln.clear_log_callback()


def test_a_providers_routes_decide_which_requests_reach_it(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    seen: list[str] = []

    def provide(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        seen.append(request.requested_url)
        handle.complete(ok_response())

    result(
        harness.runtime.set_resource_provider(
            mln.ResourceProvider(
                callback=provide,
                routes=[
                    mln.ResourceRoute(
                        flags=mln.UrlMatchFlag.GLOB, url="custom://routed/**"
                    )
                ],
            )
        )
    )

    # A style outside the routes passes through to native, which serves no
    # such scheme.
    map_handle.set_style_url("custom://elsewhere/style.json")
    harness.wait_event(mln.RuntimeEventType.MAP_LOADING_FAILED)
    assert seen == []

    map_handle.set_style_url("custom://routed/style.json")
    harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)
    assert seen == ["custom://routed/style.json"]


def test_callbacks_and_completions_run_on_the_dispatcher_thread(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    threads: dict[str, tuple[int, bool]] = {}
    submitted: list[Future[mln.CommandCompletion]] = []
    logged = threading.Event()

    def record(kind: str) -> None:
        threads.setdefault(
            kind, (threading.get_ident(), _native._on_dispatcher_thread())
        )

    def provide(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        record("provider")
        # The dispatcher runs this call, so the completion cannot run before
        # the callback that observes it is added.
        repainted = map_handle.request_repaint()
        repainted._add_internal_callback(lambda _: record("completion"))
        submitted.append(repainted)
        handle.complete(ok_response())

    def log(*_: object) -> None:
        record("log")
        logged.set()

    harness.routes["custom://dispatched.json"] = provide
    try:
        mln.set_log_callback(mln.LogHandler(log))
        map_handle.set_style_url("custom://dispatched.json")
        harness.wait_event(mln.RuntimeEventType.MAP_STYLE_LOADED)
        result(submitted[0])
        map_handle.set_style_json([])
        assert logged.wait(TIMEOUT)
    finally:
        mln.clear_log_callback()

    assert set(threads) == {"provider", "log", "completion"}
    ((ident, dispatcher),) = set(threads.values())
    assert dispatcher
    assert ident != threading.get_ident()
    assert not _native._on_dispatcher_thread()


def test_waiting_for_a_pending_future_in_a_callback_raises(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    settled = map_handle.request_repaint()
    result(settled)
    outcomes: list[object] = []
    answered = threading.Event()

    def provide(
        request: mln.ResourceRequestInfo, handle: mln.ResourceRequestHandle
    ) -> None:
        try:
            # A resolved future reads as usual.
            outcomes.append(settled.result())
            pending = map_handle.get_camera()
            for wait in (pending.result, pending.exception):
                try:
                    wait(TIMEOUT)
                except RuntimeError as error:
                    outcomes.append(error)
            outcomes.append(pending)
        finally:
            handle.complete(denied_response())
            answered.set()

    harness.routes["custom://waits.json"] = provide
    map_handle.set_style_url("custom://waits.json")
    assert answered.wait(TIMEOUT)

    completed, result_error, exception_error, pending = outcomes
    assert isinstance(completed, mln.CommandCompletion)
    assert isinstance(result_error, RuntimeError)
    assert isinstance(exception_error, RuntimeError)
    # The future resolves once the callback has returned.
    assert result(typing.cast(typing.Any, pending)).generation > 0
