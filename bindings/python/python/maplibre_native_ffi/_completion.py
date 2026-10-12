"""Terminal command values shared by generated operations and runtime APIs."""

from dataclasses import dataclass

from ._generated_values import CommandDisposition


@dataclass(frozen=True, slots=True)
class CommandCompletion:
    """Terminal result of an accepted map command."""

    disposition: CommandDisposition
    generation: int
    native_status_code: int
    diagnostic: str

    def __post_init__(self) -> None:
        object.__setattr__(self, "disposition", CommandDisposition(self.disposition))
