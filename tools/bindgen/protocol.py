"""The C API's protocol types, which every binding runtime is written against.

Each handwritten runtime reports status, carries diagnostics, and drives
completions through these declarations, so the generator recognizes them by the
names declared here and nowhere else. Every other rule reads a declaration's
shape or its metadata.
"""

from __future__ import annotations

from .model import CType

# The status enum that every fallible native call returns.
STATUS = "mln_status"
# The trailing out-parameter that carries a failed call's message.
DIAGNOSTIC = "mln_diagnostic"
# The callback registration that an asynchronous submission takes last.
COMPLETION = "mln_completion"
# The record that a completion callback receives.
COMPLETION_RESULT = "mln_completion_result"
# The record that carries a borrowed byte span.
BUFFER_VIEW = "mln_buffer_view"
# The status that a drain returns when nothing is queued.
NOT_READY = "MLN_STATUS_NOT_READY"


def declared_name(type_: CType) -> str:
    """The declaration a type names, without qualifiers."""
    return type_.declaration or type_.spelling.removeprefix("const ")


def is_status(type_: CType | None) -> bool:
    return type_ is not None and declared_name(type_) == STATUS


def is_completion(type_: CType) -> bool:
    """A `const mln_completion*` submission parameter."""
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
