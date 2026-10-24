"""Attach generated operations to their public owner for introspection."""

from contextlib import contextmanager
from typing import Any


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


@contextmanager
def _view(owner, read, convert):
    with owner._native._read_scope():
        yield convert(read())


def _wrap_response(raw, name):
    from . import _generated_owners

    return getattr(_generated_owners, name)._from_native(raw)
