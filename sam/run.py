"""sam: Compile and run a program.

© Reuben Thomas <rrt@sc3d.org> 2025.

Released under the GPL version 3, or (at your option) any later version.
"""

from time import sleep

from .assembler import compile
from .sam_ctypes import (
    SAM_ERROR_OK,
    SAM_STACK_WORDS,
    sam_dump_screen,
    sam_init,
    # sam_print_stack,
    sam_run,
    sam_traps_finish,
    sam_traps_init,
    sam_traps_process_events,
    sam_traps_window_used,
    sam_update_interval,
)
from .warnings_util import warn


def run(program: str, debug: bool, wait: bool, screen_pbm_file: str | None):
    stack, program_words = compile(program)

    if sam_traps_init() != SAM_ERROR_OK:
        exit(1)
    res = sam_init(stack, SAM_STACK_WORDS, program_words)
    if res != SAM_ERROR_OK:
        exit(1)
    # sam_print_stack()
    res = sam_run()

    if debug:
        warn(f"sam_run returns {res}")
        if sam_traps_window_used():
            if screen_pbm_file is not None:
                sam_dump_screen(screen_pbm_file)

    if wait and sam_traps_window_used():
        while not sam_traps_process_events():
            sleep(float(sam_update_interval) / 1000.0)

    sam_traps_finish()
