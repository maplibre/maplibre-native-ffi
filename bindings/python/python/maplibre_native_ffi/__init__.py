"""Low-level Python bindings generated from the MapLibre C API.

The ``maplibre-native-ffi`` distribution holds this package, and a runtime
distribution holds the native extension for one render backend:
``maplibre-native-ffi-runtime-opengl``, ``maplibre-native-ffi-runtime-metal``,
or ``maplibre-native-ffi-runtime-vulkan``. Importing the package raises
``ImportError`` unless exactly one runtime is installed.

Commands and ordered queries return ``concurrent.futures.Future`` objects.
Their results are copied before native completion returns. A command's future
resolves only when the command commits, and raises ``MaplibreError`` when it
fails or is cancelled; ``discarding`` submits a command without a future, and
native logs its failure as a warning. User done callbacks run on host worker
threads in registration order for each future; a callback can submit another
operation and wait for its result. Use ``asyncio.wrap_future`` to await a
binding future from an asyncio task.

A JSON value, annotated ``Any``, is any value that ``json.dumps`` takes, so
``set_style_layer_property("roads", "line-color", "red")`` needs no inner
quotes. A JSON parameter is always a value: ``None`` is JSON null, such as the
null that restores a layer property's default. In a record member, ``None`` is
absent, which native reads as empty, as for a filter, an expression input, or
cluster properties, where JSON null means nothing. A value that ``json.dumps``
rejects raises its error before the call reaches native. Returned JSON is the
value that ``json.loads`` reads, and absent JSON is ``None``, as JSON null is.

Every callback that Python supplies, and every completion, runs on one
dispatcher thread that the binding owns, after native queues the call, so a
callback that blocks delays every other one. Waiting for a pending binding
future in a callback raises ``RuntimeError``, since that future resolves on the
same thread. Hand work that touches thread-bound state, such as an asyncio loop
or a Tk or Qt widget, to its thread, for example with
``loop.call_soon_threadsafe``. Native code answers what a call decides itself,
so a log record is always consumed. HTTP header transforms take native header
rules, and queue locks take only native registrations, since native code
waits for both on its own threads.

Interpreter shutdown stops the dispatcher, after the call it is running
returns, and drops every call that is still queued.
"""

from ._completion import CommandCompletion
from .api import *
from .api import __all__ as _api_exports
from .errors import AbiVersionMismatchError, MaplibreError

__all__ = [
    "AbiVersionMismatchError",
    "CommandCompletion",
    "MaplibreError",
]

__all__.extend(_api_exports)
