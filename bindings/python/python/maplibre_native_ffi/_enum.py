"""Shared enum helpers for public Python value domains."""

from __future__ import annotations

from enum import IntEnum
from typing import Self


class UnknownIntEnum(IntEnum):
    """Integer enum that preserves unknown native values."""

    @property
    def native_code(self) -> int:
        """Return the C enum value for this enum member."""
        return int(self)

    @classmethod
    def _missing_(cls, value: object) -> Self | None:
        if not isinstance(value, int):
            return None
        unknown = int.__new__(cls, value)
        unknown._name_ = f"UNKNOWN_{value}"
        unknown._value_ = value
        return unknown

    @property
    def is_unknown(self) -> bool:
        """Return whether this value preserves an unknown native value."""
        return self.name.startswith("UNKNOWN_")
