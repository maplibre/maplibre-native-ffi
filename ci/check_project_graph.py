import pathlib

from ci.affected import check_graph

ROOT = pathlib.Path.cwd()


if __name__ == "__main__":
    check_graph(ROOT)
