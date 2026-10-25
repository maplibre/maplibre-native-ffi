"""Bind a parsed API once for every emitter that receives it."""

from dataclasses import replace

from .model import Api
from .schema import PLATFORMS, platforms
from .semantic import BoundApi, bind

# Each restriction of a source binds once; the entry keeps its source alive, so
# the source's id stays its own.
_bound: dict[tuple[int, frozenset[str]], tuple[Api, BoundApi]] = {}


def compile_api(
    api: Api | BoundApi, targets: frozenset[str] = frozenset(PLATFORMS)
) -> BoundApi:
    """Bind the declarations that serve at least one of a binding's targets."""
    source = api.source if isinstance(api, BoundApi) else api
    restricted = restrict(source, targets)
    if isinstance(api, BoundApi) and restricted is source:
        return api
    key = (id(source), targets)
    if key not in _bound:
        _bound[key] = (source, bind(restricted))
    return _bound[key][1]


def restrict(api: Api, targets: frozenset[str]) -> Api:
    """Leave out the functions that serve none of `targets`.

    A record that only those functions reached leaves with them.
    """
    functions = tuple(
        function for function in api.functions if platforms(function) & targets
    )
    if len(functions) == len(api.functions):
        return api
    return replace(api, functions=functions)


def served(function, targets: frozenset[str]) -> frozenset[str] | None:
    """The targets that a function serves, or None when it serves them all."""
    serves = platforms(function) & targets
    return None if serves == targets else serves
