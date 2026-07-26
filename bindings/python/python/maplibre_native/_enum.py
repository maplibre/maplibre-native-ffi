"""Shared enum helpers for public Python value domains."""

from __future__ import annotations

from enum import IntEnum
from typing import Self


class NativeIntEnum(IntEnum):
    """Integer enum whose value is the native C domain value."""

    @property
    def native_code(self) -> int:
        """Return the C enum value for this enum member."""
        return int(self)


class UnknownIntEnum(NativeIntEnum):
    """Integer enum that preserves unknown non-negative native values."""

    @classmethod
    def _missing_(cls, value: object) -> Self | None:
        if not isinstance(value, int) or value < 0:
            return None
        unknown = int.__new__(cls, value)
        unknown._name_ = f"UNKNOWN_{value}"
        unknown._value_ = value
        return unknown

    @property
    def is_unknown(self) -> bool:
        """Return whether this value preserves an unknown native value."""
        return self.name.startswith("UNKNOWN_")

    def known_native_code(self, label: str) -> int:
        """Return the native code, rejecting unknown values for setter calls."""
        if self.is_unknown:
            from .errors import InvalidArgumentError

            raise InvalidArgumentError(
                None,
                f"unknown {label} cannot be set: {int(self)}",
            )
        return int(self)
