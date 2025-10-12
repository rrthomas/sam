"""sam tests.

© Reuben Thomas <rrt@sc3d.org> 2025

Released under the GPL version 3, or (at your option) any later version.
"""

import os
import sys
from collections.abc import Iterator
from pathlib import Path

import pytest
from sam import main
from pytest import CaptureFixture, LogCaptureFixture
from testutils import failing_cli_test, passing_cli_test


if sys.version_info[:2] >= (3, 11):  # pragma: no cover
    from contextlib import chdir  # pyright: ignore
else:  # pragma: no cover
    from contextlib import contextmanager

    @contextmanager
    def chdir(path: os.PathLike[str]) -> Iterator[None]:
        old_dir = os.getcwd()
        os.chdir(path)
        try:
            yield
        finally:
            os.chdir(old_dir)


tests_dir = Path(__file__).parent.resolve() / "test-files"


# CLI tests
def test_help_option_should_produce_output(capsys: CaptureFixture[str]) -> None:
    with pytest.raises(SystemExit) as e:
        main(["--help"])
    assert e.type is SystemExit
    assert e.value.code == 0
    assert capsys.readouterr().out.find("The Simple Abstract Machine") != -1


def test_greeting(
    capsys: CaptureFixture[str],
) -> None:
    with chdir(tests_dir):
        passing_cli_test(
            capsys,
            ["--greeting=Goodbye"],
            "greeting-goodbye-expected.txt",
        )


def test_invalid_command_line_argument_causes_an_error(
    capsys: CaptureFixture[str],
    caplog: LogCaptureFixture,
) -> None:
    if sys.version_info < (3, 12, 8):
        subcommand_list = "'demo'"
    else:
        subcommand_list = "demo"
    failing_cli_test(capsys, caplog, ["--foo", "a"], f"argument SUBCOMMAND: invalid choice: 'a' (choose from {subcommand_list})")
