"""Naming rules shared by the managed-language emitters."""

from .model import CType


def pascal(name: str) -> str:
    return "".join(word[:1].upper() + word[1:] for word in name.split("_"))


def camel(name: str) -> str:
    value = pascal(name)
    return value[:1].lower() + value[1:]


def type_name(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")
