"""Language-neutral declarations extracted from the public C headers.

Typedef names remain distinct from their canonical representations. In
particular, handles with the same integer ABI remain different API types.
Source locations use paths relative to the include directory.
"""

from __future__ import annotations

import json
from dataclasses import asdict, dataclass, field


@dataclass(frozen=True)
class Location:
    path: str
    line: int
    column: int

    def __str__(self) -> str:
        return f"{self.path}:{self.line}:{self.column}"


@dataclass(frozen=True)
class CType:
    kind: str
    spelling: str
    canonical: str
    declaration: str | None = None
    const: bool = False
    volatile: bool = False
    restrict: bool = False
    pointee: CType | None = None
    element: CType | None = None
    length: int | None = None
    result: CType | None = None
    parameters: tuple[CType, ...] = ()
    variadic: bool = False


@dataclass(frozen=True)
class Parameter:
    name: str
    type: CType
    metadata: dict[str, str] = field(default_factory=dict)


@dataclass(frozen=True)
class Function:
    name: str
    return_type: CType
    parameters: tuple[Parameter, ...]
    metadata: dict[str, str]
    location: Location
    documentation: str = ""
    variadic: bool = False


@dataclass(frozen=True)
class Field:
    name: str
    type: CType
    metadata: dict[str, str]
    location: Location
    documentation: str = ""
    bit_width: int | None = None


@dataclass(frozen=True)
class Record:
    name: str
    kind: str
    fields: tuple[Field, ...]
    metadata: dict[str, str]
    location: Location
    documentation: str = ""
    complete: bool = True


@dataclass(frozen=True)
class EnumValue:
    name: str
    value: int
    documentation: str = ""


@dataclass(frozen=True)
class Enum:
    name: str
    underlying_type: CType
    values: tuple[EnumValue, ...]
    metadata: dict[str, str]
    location: Location
    documentation: str = ""


@dataclass(frozen=True)
class Typedef:
    name: str
    type: CType
    metadata: dict[str, str]
    location: Location
    documentation: str = ""
    parameters: tuple[Parameter, ...] = ()


@dataclass(frozen=True)
class Api:
    functions: tuple[Function, ...]
    records: tuple[Record, ...]
    enums: tuple[Enum, ...]
    typedefs: tuple[Typedef, ...]
    schema_version: int = 1
    runtime_exports: tuple[str, ...] = ()
    runtime_types: tuple[str, ...] = ()

    def to_json(self) -> str:
        return json.dumps(asdict(self), indent=2, sort_keys=True) + "\n"

    @property
    def public_functions(self) -> tuple[Function, ...]:
        return tuple(
            function
            for function in self.functions
            if function.name not in self.runtime_exports
        )

    @property
    def functions_by_name(self) -> dict[str, Function]:
        return {function.name: function for function in self.functions}

    @property
    def records_by_name(self) -> dict[str, Record]:
        return {record.name: record for record in self.records}

    @property
    def typedefs_by_name(self) -> dict[str, Typedef]:
        return {typedef.name: typedef for typedef in self.typedefs}


class ModelError(ValueError):
    """An input cannot be represented safely by the common model."""

    def __init__(self, diagnostics: list[str]):
        self.diagnostics = sorted(set(diagnostics))
        super().__init__("\n".join(self.diagnostics))
