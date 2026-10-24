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
