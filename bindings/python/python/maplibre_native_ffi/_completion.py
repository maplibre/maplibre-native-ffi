"""Terminal command values shared by generated operations and runtime APIs."""

from dataclasses import dataclass


@dataclass(frozen=True, slots=True)
class CommandCompletion:
    """The generations that a committed command published, which are zero
    unless it is a map command.

    A command future raises ``MaplibreError`` instead when the command fails
    or is cancelled.
    """

    generation: int
    render_update_generation: int
