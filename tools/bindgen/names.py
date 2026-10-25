"""Naming rules shared by the managed-language emitters."""

from .model import CType

# C tokens that join two words, which every binding cases as two (#993
# decision 5): GeoJson, not Geojson.
COMPOUND_WORDS = {
    "geojson": ("geo", "json"),
    "opengl": ("open", "gl"),
    "tilejson": ("tile", "json"),
    "webgl": ("web", "gl"),
    "webgpu": ("web", "gpu"),
}


def words(name: str) -> list[str]:
    """The words of a C snake-case name, with joined tokens split apart."""
    return [
        word
        for token in name.split("_")
        for word in COMPOUND_WORDS.get(token.lower(), (token,))
    ]


def pascal(name: str) -> str:
    return "".join(word[:1].upper() + word[1:] for word in words(name))


def camel(name: str) -> str:
    value = pascal(name)
    return value[:1].lower() + value[1:]


def type_name(type_: CType) -> str:
    return type_.declaration or type_.spelling.removeprefix("const ")
