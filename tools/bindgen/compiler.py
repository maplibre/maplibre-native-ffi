"""Shared semantic compilation entry point for every static backend."""

from .model import Api
from .semantic import BoundApi, bind


def compile_api(api: Api | BoundApi) -> BoundApi:
    return api if isinstance(api, BoundApi) else bind(api)
