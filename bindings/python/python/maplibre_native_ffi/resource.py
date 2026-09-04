"""Resource request, response, transform, and provider APIs."""

from __future__ import annotations

import threading
import weakref
from collections.abc import Callable
from dataclasses import dataclass
from typing import Any

from ._enum import NativeIntEnum, UnknownIntEnum
from ._lifecycle import ContextHandleMixin, WarnUnclosedMixin

_REQUEST_HANDLE_CREATE_KEY = object()


class ResourceKind(UnknownIntEnum):
    """Network resource kind passed to resource callbacks."""

    UNKNOWN = 0
    STYLE = 1
    SOURCE = 2
    TILE = 3
    GLYPHS = 4
    SPRITE_IMAGE = 5
    SPRITE_JSON = 6
    IMAGE = 7


class ResourceLoadingMethod(UnknownIntEnum):
    """Resource loading method requested by MapLibre Native."""

    ALL = 0
    CACHE_ONLY = 1
    NETWORK_ONLY = 2


class ResourcePriority(UnknownIntEnum):
    """Resource request priority."""

    REGULAR = 0
    LOW = 1


class ResourceUsage(UnknownIntEnum):
    """Resource request usage."""

    ONLINE = 0
    OFFLINE = 1


class ResourceStoragePolicy(UnknownIntEnum):
    """Resource cache storage policy."""

    PERMANENT = 0
    VOLATILE = 1


class ResourceResponseStatus(NativeIntEnum):
    """Status for a resource provider response."""

    OK = 0
    ERROR = 1
    NO_CONTENT = 2
    NOT_MODIFIED = 3


class ResourceErrorReason(UnknownIntEnum):
    """Resource error reason used in provider responses and events."""

    NONE = 0
    NOT_FOUND = 1
    SERVER = 2
    CONNECTION = 3
    RATE_LIMIT = 4
    OTHER = 5


class ResourceProviderDecision(NativeIntEnum):
    """Decision returned by a resource provider callback."""

    PASS_THROUGH = 0
    HANDLE = 1


@dataclass(frozen=True, slots=True)
class ByteRange:
    """Byte range requested for a network resource."""

    start: int
    end: int

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> ByteRange:
        return cls(start=raw["start"], end=raw["end"])


@dataclass(frozen=True, slots=True)
class ResourceRequest:
    """Copied request passed to a resource provider callback."""

    requested_url: str
    """URL entering the network layer, preserving configured scheme aliases."""

    resolved_url: str
    """URL to fetch, after tile server normalization."""

    kind: ResourceKind
    loading_method: ResourceLoadingMethod
    priority: ResourcePriority
    usage: ResourceUsage
    storage_policy: ResourceStoragePolicy
    range: ByteRange | None
    prior_modified_unix_ms: int | None
    prior_expires_unix_ms: int | None
    prior_etag: str | None
    prior_data: bytes

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> ResourceRequest:
        raw_range = raw["range"]
        return cls(
            requested_url=raw["requested_url"],
            resolved_url=raw["resolved_url"],
            kind=ResourceKind(raw["kind"]),
            loading_method=ResourceLoadingMethod(raw["loading_method"]),
            priority=ResourcePriority(raw["priority"]),
            usage=ResourceUsage(raw["usage"]),
            storage_policy=ResourceStoragePolicy(raw["storage_policy"]),
            range=ByteRange._from_native(raw_range) if raw_range is not None else None,
            prior_modified_unix_ms=raw["prior_modified_unix_ms"],
            prior_expires_unix_ms=raw["prior_expires_unix_ms"],
            prior_etag=raw["prior_etag"],
            prior_data=raw["prior_data"],
        )


@dataclass(frozen=True, slots=True)
class ResourceTransformRequest:
    """Copied request passed to a resource transform callback."""

    kind: ResourceKind
    url: str

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> ResourceTransformRequest:
        return cls(kind=ResourceKind(raw["kind"]), url=raw["url"])


@dataclass(frozen=True, slots=True)
class HttpHeaderTransformRequest:
    """Copied request passed to an outgoing HTTP header transform."""

    kind: ResourceKind
    url: str

    @classmethod
    def _from_native(cls, raw: dict[str, Any]) -> HttpHeaderTransformRequest:
        return cls(kind=ResourceKind(raw["kind"]), url=raw["url"])


@dataclass(frozen=True, slots=True)
class HttpHeader:
    """One owned outgoing HTTP request header."""

    name: str
    value: str


@dataclass(frozen=True, slots=True)
class ResourceResponse:
    """Response used to complete a handled resource request."""

    status: ResourceResponseStatus = ResourceResponseStatus.OK
    error_reason: ResourceErrorReason = ResourceErrorReason.NONE
    bytes: bytes = b""
    error_message: str | None = None
    must_revalidate: bool = False
    modified_unix_ms: int | None = None
    expires_unix_ms: int | None = None
    etag: str | None = None
    retry_after_unix_ms: int | None = None

    def _to_native(self) -> dict[str, Any]:
        return {
            "status": self.status.native_code,
            "error_reason": self.error_reason.native_code,
            "bytes": self.bytes,
            "error_message": self.error_message,
            "must_revalidate": self.must_revalidate,
            "modified_unix_ms": self.modified_unix_ms,
            "expires_unix_ms": self.expires_unix_ms,
            "etag": self.etag,
            "retry_after_unix_ms": self.retry_after_unix_ms,
        }


class ResourceRequestHandle(WarnUnclosedMixin, ContextHandleMixin):
    """One-shot handle for a resource provider request selected for handling."""

    _handle_name = "ResourceRequestHandle"

    def __init__(self, native: Any, *, _create_key: object | None = None) -> None:
        if _create_key is not _REQUEST_HANDLE_CREATE_KEY:
            msg = "ResourceRequestHandle instances are created by resource providers"
            raise TypeError(msg)
        self._native = native
        self._closed = False
        self._cancel_callback: ResourceCancelCallback | None = None
        self._cancel_lock = threading.Lock()

    @classmethod
    def _from_native(cls, native: Any) -> ResourceRequestHandle:
        return cls(native, _create_key=_REQUEST_HANDLE_CREATE_KEY)

    @property
    def closed(self) -> bool:
        """Return whether this request handle has been completed or released."""
        return self._closed

    def complete(self, response: ResourceResponse) -> None:
        """Complete this request with a copied response."""
        if self._closed:
            from .errors import InvalidStateError

            raise InvalidStateError(
                None,
                "resource request handle is already closed",
            )
        native_response = response._to_native()
        self._native.validate_completion_response(native_response)
        try:
            self._native.complete(native_response)
        except BaseException:
            self.close()
            raise
        self._closed = True
        self._take_cancel_callback()

    def is_cancelled(self) -> bool:
        """Return whether native code has cancelled the request."""
        if self._closed:
            from .errors import InvalidStateError

            raise InvalidStateError(
                None,
                "resource request handle is already closed",
            )
        return bool(self._native.is_cancelled())

    def set_cancel_callback(self, callback: ResourceCancelCallback) -> None:
        """Register this request's one cancellation callback.

        The callback takes no arguments and runs at most once, on the thread
        that cancels the request. That thread is the runtime owner thread inside
        a map or runtime call and a MapLibre thread otherwise. A request the
        provider completed never runs it. Registering on an already cancelled
        request runs the callback before this call returns.

        A request accepts one registration: a second one raises
        :class:`InvalidStateError`, as does a registration on a handle that
        :meth:`complete` or :meth:`close` already closed.

        The callback may call :meth:`complete`, which reports an invalid state
        for a cancelled request, and :meth:`close` on this handle. Runtime and
        map calls belong outside the callback. An exception raised by the
        callback stays inside it: the binding discards the exception rather
        than returning it to MapLibre.

        This handle owns the callback until it runs or the handle is completed
        or closed. Native code reaches the callback through a weak reference to
        this handle, so a callback that captures the handle forms a cycle the
        garbage collector can still reclaim.
        """
        if self._closed:
            from .errors import InvalidStateError

            raise InvalidStateError(
                None,
                "resource request handle is already closed",
            )
        if not callable(callback):
            msg = "cancel callback must be callable"
            raise TypeError(msg)
        with self._cancel_lock:
            if self._cancel_callback is not None:
                from .errors import InvalidStateError

                raise InvalidStateError(
                    None,
                    "resource request handle already has a cancel callback",
                )
            self._cancel_callback = callback
        handle_ref = weakref.ref(self)

        def dispatch() -> None:
            handle = handle_ref()
            if handle is None:
                return
            pending = handle._take_cancel_callback()
            if pending is not None:
                pending()

        try:
            self._native.set_cancel_callback(dispatch)
        except BaseException:
            self._take_cancel_callback()
            raise

    def _take_cancel_callback(self) -> ResourceCancelCallback | None:
        with self._cancel_lock:
            pending = self._cancel_callback
            self._cancel_callback = None
            return pending

    def close(self) -> None:
        """Release this request handle without completing it."""
        if self._closed:
            return
        self._native.close()
        self._closed = True
        self._take_cancel_callback()


ResourceCancelCallback = Callable[[], None]
ResourceTransformCallback = Callable[[ResourceTransformRequest], str | None]
HttpHeaderTransformCallback = Callable[[HttpHeaderTransformRequest], list[HttpHeader]]
ResourceProviderCallback = Callable[
    [ResourceRequest, ResourceRequestHandle], ResourceProviderDecision
]


def _adapt_resource_transform_callback(
    callback: ResourceTransformCallback,
) -> Callable[[dict[str, Any]], str | None]:
    def adapted(raw_request: dict[str, Any]) -> str | None:
        return callback(ResourceTransformRequest._from_native(raw_request))

    return adapted


def _adapt_http_header_transform_callback(
    callback: HttpHeaderTransformCallback,
) -> Callable[[dict[str, Any]], list[dict[str, str]]]:
    def adapted(raw_request: dict[str, Any]) -> list[dict[str, str]]:
        return [
            {"name": header.name, "value": header.value}
            for header in callback(HttpHeaderTransformRequest._from_native(raw_request))
        ]

    return adapted


def _adapt_resource_provider_callback(
    callback: ResourceProviderCallback,
) -> Callable[[dict[str, Any], Any], int]:
    def adapted(raw_request: dict[str, Any], native_handle: Any) -> int:
        handle = ResourceRequestHandle._from_native(native_handle)
        try:
            decision = ResourceProviderDecision(
                callback(ResourceRequest._from_native(raw_request), handle)
            )
        except BaseException:
            if not handle.closed:
                handle.close()
            raise
        if decision is not ResourceProviderDecision.HANDLE and not handle.closed:
            handle.close()
        return decision.native_code

    return adapted


__all__ = [
    "ByteRange",
    "HttpHeader",
    "HttpHeaderTransformCallback",
    "HttpHeaderTransformRequest",
    "ResourceCancelCallback",
    "ResourceErrorReason",
    "ResourceKind",
    "ResourceLoadingMethod",
    "ResourcePriority",
    "ResourceProviderCallback",
    "ResourceProviderDecision",
    "ResourceRequest",
    "ResourceRequestHandle",
    "ResourceResponse",
    "ResourceResponseStatus",
    "ResourceStoragePolicy",
    "ResourceTransformCallback",
    "ResourceTransformRequest",
    "ResourceUsage",
]
