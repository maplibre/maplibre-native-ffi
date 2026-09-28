# Python binding generation

The Python emitter accounts for all 264 public C declarations: 222 generated
operations and 42 verified support relationships. The public API, native calls,
recursive values, owner classes, callback descriptors, and type stubs derive
from the same resolved header model.

## Runtime boundary

Generated PyO3 methods call C directly. Public operation methods expose typed
futures and copied values. A class-construction helper preserves signatures for
introspection and API documentation without a forwarding call layer.

The handwritten runtime owns library loading, native handle state, callback
roots, diagnostic conversion, and future delivery. Garbage collection visits
owner-held callback roots, including callbacks that capture their owner. Native
release retires those roots after quiescence.

User future callbacks run on host workers. Internal result conversion stays
inline, and the shared Rust callback admission guard enforces reentry rules.
Resource request completion permits retry after native rejection and retains
accepted caller ownership until close. Scoped GPU access uses the native view
gate to retain resources through the Python callback.

## Evidence

The current release extension builds against this checkout's macOS ARM64 Metal
library. Its host suite passes 168 tests; 24 tests select other render backends.
The suite includes copied event messages after batch release, captured-owner
collection, resource completion retry, inline close, scoped GPU retirement, and
awaiting a query from a future callback. Ruff, configured type checks, and Pdoc
generation pass.

Generator fixtures cover new declarations, nested values, nullable counted
arrays, fixed-width integer aliases, escaped identifiers, and fail-closed
rejection of incomplete ownership or input reconstruction contracts.
