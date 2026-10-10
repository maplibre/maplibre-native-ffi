"""Low-level Python bindings generated from the MapLibre C API.

Commands and ordered queries return ``concurrent.futures.Future`` objects.
Their results are copied before native completion returns. User done callbacks
run on host worker threads in registration order for each future; a callback can
submit another operation and wait for its result. Use ``asyncio.wrap_future`` to
await a binding future from an asyncio task.
"""

from . import _loader as _loader
from ._completion import CommandCompletion, CommandDisposition
from .api import *
from .api import __all__ as _api_exports
from .errors import (
    BusyError,
    CancelledError,
    InvalidArgumentError,
    InvalidStateError,
    MaplibreError,
    NativeError,
    NotFoundError,
    NotReadyError,
    TargetLostError,
    UnknownStatusError,
    UnsupportedFeatureError,
    WrongThreadError,
)

__all__ = [
    "BusyError",
    "CancelledError",
    "CommandCompletion",
    "CommandDisposition",
    "InvalidArgumentError",
    "InvalidStateError",
    "MaplibreError",
    "NativeError",
    "NotFoundError",
    "NotReadyError",
    "TargetLostError",
    "UnknownStatusError",
    "UnsupportedFeatureError",
    "WrongThreadError",
]

__all__.extend(_api_exports)
