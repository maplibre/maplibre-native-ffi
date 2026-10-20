"""The C API's protocol types, which every binding runtime is written against.

Each handwritten runtime reports status, carries diagnostics, and drives
completions through these declarations, so the generator recognizes them by the
names declared here and nowhere else. Every other rule reads a declaration's
shape or its metadata.
"""

from __future__ import annotations

from .model import CType

STATUS = "mln_status"
DIAGNOSTIC = "mln_diagnostic"
COMPLETION = "mln_completion"
COMPLETION_RESULT = "mln_completion_result"
# Its members that count an array result's elements and give their byte
# stride, which is the size of one element in the native build.
COMPLETION_VALUE = "value"
COMPLETION_VALUE_COUNT = "value_count"
COMPLETION_VALUE_SIZE = "value_size"
BUFFER_VIEW = "mln_buffer_view"
NOT_READY = "MLN_STATUS_NOT_READY"
# Queue delivery: the queue and batch handles, one drained delivery, its payload
# union, and the batch view that a runtime walks by the delivery stride.
DELIVERY_QUEUE = "mln_delivery_queue"
DELIVERY_BATCH = "mln_delivery_batch"
DELIVERY = "mln_delivery"
DELIVERY_PAYLOAD = "mln_delivery_payload"
DELIVERY_BATCH_VIEW = "mln_delivery_batch_view"


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
