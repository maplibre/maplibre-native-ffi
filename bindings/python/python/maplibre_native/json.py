"""JSON value trees that preserve numeric shape and duplicate object keys."""

from __future__ import annotations

from dataclasses import dataclass
import math
from typing import Any, TypeAlias


@dataclass(frozen=True, slots=True)
class JsonUInt:
    """Unsigned JSON integer value."""

    value: int

    def __post_init__(self) -> None:
        if self.value < 0:
            msg = "JsonUInt value must be non-negative"
            raise ValueError(msg)


@dataclass(frozen=True, slots=True)
class JsonInt:
    """Signed JSON integer value."""

    value: int


@dataclass(frozen=True, slots=True)
class JsonDouble:
    """Finite JSON double value."""

    value: float

    def __post_init__(self) -> None:
        if not math.isfinite(self.value):
            msg = "JsonDouble value must be finite"
            raise ValueError(msg)


@dataclass(frozen=True, slots=True)
class JsonMember:
    """Ordered JSON object member. Duplicate keys are preserved."""

    key: str
    value: JsonValue


@dataclass(frozen=True, slots=True)
class JsonArray:
    """Ordered JSON array value."""

    values: tuple[JsonValue, ...]


@dataclass(frozen=True, slots=True)
class JsonObject:
    """Ordered JSON object value that preserves duplicate keys."""

    members: tuple[JsonMember, ...]


JsonScalar: TypeAlias = None | bool | str | JsonUInt | JsonInt | JsonDouble
JsonValue: TypeAlias = JsonScalar | JsonArray | JsonObject


def from_python(value: Any) -> JsonValue:
    """Convert ordinary Python values to explicit JSON values.

    Python `dict` preserves insertion order, but cannot represent duplicate
    keys. Use `JsonObject` with explicit `JsonMember` values when duplicate
    object members matter.
    Python `int` converts to `JsonInt`; use `JsonUInt` to preserve unsigned
    integer shape.
    """
    if value is None or isinstance(value, bool | str):
        return value
    if isinstance(value, int):
        return JsonInt(value)
    if isinstance(value, float):
        return JsonDouble(value)
    if isinstance(value, list | tuple):
        return JsonArray(tuple(from_python(item) for item in value))
    if isinstance(value, dict):
        return JsonObject(
            tuple(
                JsonMember(str(key), from_python(item)) for key, item in value.items()
            )
        )
    msg = f"unsupported JSON value: {type(value).__name__}"
    raise TypeError(msg)


def to_python(value: JsonValue) -> Any:
    """Convert a JSON value to Python containers.

    `JsonObject` converts to an ordered list of `(key, value)` pairs so duplicate
    keys survive the conversion.
    """
    if isinstance(value, JsonUInt | JsonInt | JsonDouble):
        return value.value
    if isinstance(value, JsonArray):
        return [to_python(item) for item in value.values]
    if isinstance(value, JsonObject):
        return [(member.key, to_python(member.value)) for member in value.members]
    return value


__all__ = [
    "JsonArray",
    "JsonDouble",
    "JsonInt",
    "JsonMember",
    "JsonObject",
    "JsonScalar",
    "JsonUInt",
    "JsonValue",
    "from_python",
    "to_python",
]
