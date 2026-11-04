"""The C API's protocol types, which every binding runtime is written against.

Each handwritten runtime reports status, carries diagnostics, and drives
completions through these declarations, so the generator recognizes them by the
names declared here and nowhere else. Every other rule reads a declaration's
shape or its metadata.
"""

from __future__ import annotations

from .model import Api, CType

STATUS = "mln_status"
STATUS_OK = "MLN_STATUS_OK"
DIAGNOSTIC = "mln_diagnostic"
COMPLETION = "mln_completion"
COMPLETION_RESULT = "mln_completion_result"
# Its members that count an array result's elements and give their byte
# stride, which is the size of one element in the native build.
COMPLETION_VALUE = "value"
COMPLETION_VALUE_COUNT = "value_count"
COMPLETION_VALUE_SIZE = "value_size"
BUFFER_VIEW = "mln_buffer_view"
DELIVERY_QUEUE = "mln_delivery_queue"
# The query of the C ABI version, which every runtime's version check calls, so
# calling it skips the check.
ABI_VERSION = "mln_c_version"
# The epoch of the headers, which each runtime compares with the library's
# before its first call.
C_VERSION = "MLN_C_VERSION"
# The null handle, which only a runtime writes and reads, since every binding
# presents handles as its own objects.
HANDLE_NULL = "MLN_HANDLE_NULL"
# The constants that the runtimes use, which stay out of every public API.
RUNTIME_CONSTANTS = frozenset({C_VERSION, HANDLE_NULL})
# The enum that names the kinds of handle a runtime event reports as its source,
# and the prefix of its members, each `<prefix><HANDLE>` for handle `mln_<handle>`.
RUNTIME_EVENT_SOURCE = "mln_runtime_event_source_type"
RUNTIME_EVENT_SOURCE_PREFIX = "MLN_RUNTIME_EVENT_SOURCE_"
# The record of a render session abandon, whose layout a runtime reads.
RENDER_ABANDON_RESULT = "mln_render_abandon_result"


def c_version(api: Api) -> int | None:
    """The epoch that the headers declare, when they define one."""
    constant = api.constants_by_name.get(C_VERSION)
    return None if constant is None else constant.value


def declared_name(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")


def is_status(type_: CType | None) -> bool:
    return type_ is not None and declared_name(type_) == STATUS


def is_completion(type_: CType) -> bool:
    return bool(
        type_.kind == "pointer"
        and type_.pointee
        and type_.pointee.const
        and type_.pointee.declaration == COMPLETION
    )


def is_buffer_view(type_: CType) -> bool:
    return (
        type_.declaration == BUFFER_VIEW
        or type_.canonical.removeprefix("const ") == f"struct {BUFFER_VIEW}"
    )
