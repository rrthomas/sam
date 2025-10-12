"""sam: Assembler.

© Reuben Thomas <rrt@sc3d.org> 2025.

Released under the GPL version 3, or (at your option) any later version.
"""

import struct

import yaml
import yaml_include


yaml.add_constructor("!include", yaml_include.Constructor(base_dir="."))

INDENT = 2


def indented(s, level=0):
    return f"{' ' * (level * INDENT)}{s},"


def instruction(inst, op=None):
    s = f"SAM_INSN_{inst.upper()}"
    if op is not None:
        s = f"LSHIFT({op}, SAM_OP_SHIFT) | {s}"
    return s


def compile_inst(inst, level, pc):
    labels = {}

    if isinstance(inst, dict):  # label
        assert len(inst) == 1
        (label, inst), *_ = inst.items()
        labels[label] = pc

    if isinstance(inst, list):  # sub-routine
        words, sub_labels, nwords = compile_list(inst, level + 1, pc + 1)
        words.insert(0, indented(instruction("bra", nwords), level))
        words.append(indented(instruction("ket", nwords), level))
        labels.update(sub_labels)
        return words, labels, nwords + 2

    toks = inst.split(" ")
    inst = toks[0]
    operand = None

    if inst in ("int", "link", "float", "trap", "push"):
        if len(toks) != 2:
            raise ValueError(f"{inst} needs an operand")
        operand = toks[1]

        if inst in ("int", "link"):
            return [indented(instruction(inst, operand), level)], labels, 1
        elif inst == "float":
            f = struct.pack("!f", float(operand)).hex()
            return (
                [
                    indented(instruction("float", f"0x{f} >> SAM_OP_SHIFT"), level),
                    indented(instruction("_float", f"0x{f} & SAM_OP_MASK"), level),
                ],
                labels,
                2,
            )
        elif inst == "trap":
            return [indented(instruction("trap", f"TRAP_{operand}"), level)], labels, 1
        else:  # inst == "push"
            return (
                [
                    indented(instruction("push", f"{operand} >> SAM_OP_SHIFT"), level),
                    indented(instruction("_push", f"{operand} & SAM_OP_MASK"), level),
                ],
                labels,
                2,
            )
    else:
        if len(toks) != 1:
            raise ValueError(f"unexpected operand for {inst}")
        return [indented(instruction(inst), level)], labels, 1


def compile_list(l, level=0, pc0=0):
    labels, words = {}, []
    pc = pc0
    for inst in l:
        sub_words, sub_labels, n_sub_words = compile_inst(inst, level, pc)
        words.extend(sub_words)
        labels.update(sub_labels)
        pc += n_sub_words
    return words, labels, pc - pc0


def compile(program_file: str) -> tuple[list[str], int]:
    prog = yaml.full_load(open(program_file).read())

    c_source = []
    words, labels, n_words = compile_list(prog)

    for label, n in labels.items():
        c_source.append(f"#define {label} {n}")
    for word in words:
        c_source.append(word)

    c_source.append(indented(instruction("link", 1)))
    c_source.append(indented(instruction("link", 1)))

    return c_source, n_words + 2
