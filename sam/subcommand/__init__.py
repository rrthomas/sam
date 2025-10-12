"""sam: Sub-command loader.

© Reuben Thomas <rrt@sc3d.org> 2025.

Released under the GPL version 3, or (at your option) any later version.
"""

import importlib.util
import os
import sys
from types import ModuleType


# Adapted from https://docs.python.org/3/library/importlib.html#importing-a-source-file-directly
def import_from_path(file_path: str)  -> ModuleType:
    module_name = os.path.splitext(file_path)[0]
    spec = importlib.util.spec_from_file_location(module_name, file_path)
    assert spec is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    loader = spec.loader
    assert loader is not None
    loader.exec_module(module)
    return module

def commands():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    modules = [
        os.path.join(this_dir, f)
        for f in os.listdir(this_dir) if not f.startswith("__")
    ]

    commands = []
    for m in modules:
        commands.append(import_from_path(m))
    return commands
