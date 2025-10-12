"""Tests utility routines.

Copyright (c) Reuben Thomas 2023-2025.

Released under the GPL version 3, or (at your option) any later version.
"""

import filecmp
import os
from tempfile import TemporaryDirectory
from typing import Optional

import pytest
from pytest import CaptureFixture, LogCaptureFixture

from sam import main


def passing_cli_test(
    capsys: CaptureFixture[str],
    args: list[str],
    expected: str,
) -> None:
    main(args)
    with open(expected, encoding="utf-8") as fh:
        expected_text = fh.read()
        assert capsys.readouterr().out == expected_text


def failing_cli_test(
    capsys: CaptureFixture[str],
    caplog: LogCaptureFixture,
    args: list[str],
    expected: str,
) -> None:
    with pytest.raises(SystemExit) as e:
        passing_cli_test(capsys, args, "")
    assert e.type is SystemExit
    assert e.value.code != 0
    err = capsys.readouterr().err
    log = caplog.messages
    match = err.find(expected) != -1 or any(msg.find(expected) != -1 for msg in log)
    if not match:  # pragma: no cover
        print(err)
        print(log)
    assert match
