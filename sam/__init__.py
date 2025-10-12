"""sam: Main module.

© Reuben Thomas <rrt@sc3d.org> 2025.

Released under the GPL version 3, or (at your option) any later version.
"""

import argparse
import importlib.metadata
import logging
import os
import sys
import warnings

from .run import run
from .subcommand import commands
from .warnings_util import die, simple_warning


VERSION = importlib.metadata.version("sam")


def main(argv: list[str] = sys.argv[1:]) -> None:
    # Command-line arguments
    parser = argparse.ArgumentParser(
        description="The Super-Awesome Machine",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-V",
        "--version",
        action="version",
        version=f"""%(prog)s {VERSION}
© 2025 Reuben Thomas <rrt@sc3d.org>
https://github.com/rrthomas/sam
Distributed under the GNU General Public License version 3, or (at
your option) any later version. There is no warranty.""",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="output debug information to standard error",
    )
    parser.add_argument(
        "--wait",
        action="store_true",
        help="wait for user to close window on termination",
    )
    parser.add_argument(
        "--dump-screen",
        metavar="FILE",
        help="output screen to PBM file FILE",
    )
    parser.add_argument(
        "program",
        metavar="PROGRAM",
        help="program to compile and run",
    )
    warnings.showwarning = simple_warning(parser.prog)

    # Locate and load sub-commands
    command_list = commands()
    if len(command_list) > 0:
        subparsers = parser.add_subparsers(title="subcommands", metavar="SUBCOMMAND")
        for command in command_list:
            command.add_subparser(subparsers)

    args = parser.parse_args(argv)

    # Run command
    try:
        if "func" in args:
            args.func(args)
        else:
            run(args.program, args.debug, args.wait, args.dump_screen)
    except Exception as err:
        if "DEBUG" in os.environ:
            logging.error(err, exc_info=True)
        else:
            die(f"{err}")
        sys.exit(1)
