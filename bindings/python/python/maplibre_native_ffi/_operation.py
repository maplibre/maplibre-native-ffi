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
                # A classmethod object isn't callable, but a constructor such
                # as ``create`` belongs on the owner too.
                if (
                    not name.startswith("_")
                    and (callable(method) or isinstance(method, classmethod))
                    and name not in vars(cls)
                ):
                    setattr(cls, name, method)


def _adopt_value(raw, owner, parent=None, ready=None):
    from . import _generated_owners

    adopt = getattr(_generated_owners, owner)._from_native
    return adopt(raw, parent) if ready is None else adopt(raw, parent, ready)


def _answer(owner, call):
    """Runs an answer, which ends the handle whatever it returns. Native ends
    a live handle on every outcome, and an answer that fails before it reaches
    native, such as on an argument that fails to convert, releases the handle."""
    try:
        return call()
    except BaseException:
        native = owner._native
        if not native._is_closed and not native._is_borrowed:
            native.release()
        raise


@contextmanager
def _view(owner, read, convert):
    with owner._native._read_scope():
        yield convert(read())
