"""Bind a parsed API once for every emitter that receives it."""

from .model import Api
from .semantic import BoundApi, bind


def compile_api(api: Api | BoundApi) -> BoundApi:
    return api if isinstance(api, BoundApi) else bind(api)
