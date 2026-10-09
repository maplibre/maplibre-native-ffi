"""Attach generated operations to their public owner for introspection."""

from contextlib import ExitStack
from typing import Any

from ._future import map_future


class GeneratedOperations:
    """Keep generated method signatures visible on the public handle class."""

    def __init_subclass__(cls, **kwargs: Any) -> None:
        super().__init_subclass__(**kwargs)
        from . import _generated_operations, _generated_values

        # The generated modules import owner classes only for type checking,
        # so their string annotations resolve through these entries.
        vars(_generated_operations)[cls.__name__] = cls
        vars(_generated_values)[cls.__name__] = cls
        for base in cls.__bases__:
            if not issubclass(base, GeneratedOperations):
                continue
            for name, method in vars(base).items():
                if (
                    not name.startswith("_")
                    and callable(method)
                    and name not in vars(cls)
                ):
                    setattr(cls, name, method)


def _adopt_value(raw, owner, parent=None):
    from . import _generated_owners

    return getattr(_generated_owners, owner)._from_native(raw, parent)


def _adopt_future(source, owner, parent):
    return map_future(source, lambda raw: _adopt_value(raw, owner, parent))


def _with_view(owner, read, convert, callback):
    parents = []
    while owner is not None:
        parents.append(owner)
        owner = getattr(owner, "_parent", None)
    with ExitStack() as scope:
        for parent in reversed(parents):
            scope.enter_context(parent._native._read_scope())
        return callback(convert(read()))


def _wrap_response(raw, name):
    from . import _generated_owners

    return getattr(_generated_owners, name)._from_native(raw)
