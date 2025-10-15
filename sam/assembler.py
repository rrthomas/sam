"""sam: Assembler.

© Reuben Thomas <rrt@sc3d.org> 2025.

Released under the GPL version 3, or (at your option) any later version.
"""

import struct

import yaml
import yaml_include

from .sam_ctypes import (
    SAM_INSN__FLOAT,
    SAM_INSN__PUSH,
    SAM_INSN_ADD,
    SAM_INSN_AND,
    SAM_INSN_ARSH,
    SAM_INSN_BRA,
    SAM_INSN_COS,
    SAM_INSN_DEG,
    SAM_INSN_DIV,
    SAM_INSN_DO,
    SAM_INSN_DUP,
    SAM_INSN_EQ,
    SAM_INSN_F2I,
    SAM_INSN_FLOAT,
    SAM_INSN_HALT,
    SAM_INSN_I2F,
    SAM_INSN_IDUP,
    SAM_INSN_IF,
    SAM_INSN_INT,
    SAM_INSN_ISET,
    SAM_INSN_KET,
    SAM_INSN_LINK,
    SAM_INSN_LOOP,
    SAM_INSN_LSH,
    SAM_INSN_LT,
    SAM_INSN_MUL,
    SAM_INSN_NEG,
    SAM_INSN_NOP,
    SAM_INSN_NOT,
    SAM_INSN_OR,
    SAM_INSN_POP,
    SAM_INSN_POW,
    SAM_INSN_PUSH,
    SAM_INSN_RAD,
    SAM_INSN_REM,
    SAM_INSN_RSH,
    SAM_INSN_SIN,
    SAM_INSN_SWAP,
    SAM_INSN_TRAP,
    SAM_INSN_WHILE,
    SAM_INSN_XOR,
    SAM_OP_MASK,
    SAM_OP_SHIFT,
    TRAP_BLACK,
    TRAP_CLEARSCREEN,
    TRAP_DISPLAY_HEIGHT,
    TRAP_DISPLAY_WIDTH,
    TRAP_DRAWBITMAP,
    TRAP_DRAWCIRCLE,
    TRAP_DRAWLINE,
    TRAP_DRAWRECT,
    TRAP_DRAWROUNDRECT,
    TRAP_FILLCIRCLE,
    TRAP_FILLRECT,
    TRAP_INVERTRECT,
    TRAP_SETDOT,
    TRAP_WHITE,
    sam_uword_t,
    sam_word_t,
)


yaml.add_constructor("!include", yaml_include.Constructor(base_dir="."))

INDENT = 2

insts = {
    "_float": SAM_INSN__FLOAT,
    "_push": SAM_INSN__PUSH,
    "add": SAM_INSN_ADD,
    "and": SAM_INSN_AND,
    "arsh": SAM_INSN_ARSH,
    "bra": SAM_INSN_BRA,
    "cos": SAM_INSN_COS,
    "deg": SAM_INSN_DEG,
    "div": SAM_INSN_DIV,
    "do": SAM_INSN_DO,
    "dup": SAM_INSN_DUP,
    "eq": SAM_INSN_EQ,
    "f2i": SAM_INSN_F2I,
    "float": SAM_INSN_FLOAT,
    "halt": SAM_INSN_HALT,
    "i2f": SAM_INSN_I2F,
    "idup": SAM_INSN_IDUP,
    "if": SAM_INSN_IF,
    "int": SAM_INSN_INT,
    "iset": SAM_INSN_ISET,
    "ket": SAM_INSN_KET,
    "link": SAM_INSN_LINK,
    "loop": SAM_INSN_LOOP,
    "lsh": SAM_INSN_LSH,
    "lt": SAM_INSN_LT,
    "mul": SAM_INSN_MUL,
    "neg": SAM_INSN_NEG,
    "nop": SAM_INSN_NOP,
    "not": SAM_INSN_NOT,
    "or": SAM_INSN_OR,
    "pop": SAM_INSN_POP,
    "pow": SAM_INSN_POW,
    "push": SAM_INSN_PUSH,
    "rad": SAM_INSN_RAD,
    "rem": SAM_INSN_REM,
    "rsh": SAM_INSN_RSH,
    "sin": SAM_INSN_SIN,
    "swap": SAM_INSN_SWAP,
    "trap": SAM_INSN_TRAP,
    "while": SAM_INSN_WHILE,
    "xor": SAM_INSN_XOR,
}

traps = {
    "BLACK": TRAP_BLACK,
    "CLEARSCREEN": TRAP_CLEARSCREEN,
    "DISPLAY_HEIGHT": TRAP_DISPLAY_HEIGHT,
    "DISPLAY_WIDTH": TRAP_DISPLAY_WIDTH,
    "DRAWBITMAP": TRAP_DRAWBITMAP,
    "DRAWCIRCLE": TRAP_DRAWCIRCLE,
    "DRAWLINE": TRAP_DRAWLINE,
    "DRAWRECT": TRAP_DRAWRECT,
    "DRAWROUNDRECT": TRAP_DRAWROUNDRECT,
    "FILLCIRCLE": TRAP_FILLCIRCLE,
    "FILLRECT": TRAP_FILLRECT,
    "INVERTRECT": TRAP_INVERTRECT,
    "SETDOT": TRAP_SETDOT,
    "WHITE": TRAP_WHITE,
}


def instruction(inst: str, op: sam_word_t | None = None) -> sam_word_t:
    w = insts[inst.lower()]
    if op is not None:
        w = (op.value << SAM_OP_SHIFT.value) | w
    return sam_word_t(w)


def compile_inst(
    inst, pc: sam_uword_t
) -> tuple[list[sam_word_t], dict[str, sam_uword_t], int]:
    labels: dict[str, sam_uword_t] = {}

    if isinstance(inst, dict):  # label
        assert len(inst) == 1
        (label, inst), *_ = inst.items()
        labels[label] = pc

    if isinstance(inst, list):  # sub-routine
        words, sub_labels, nwords = compile_list(inst, sam_uword_t(pc.value + 1))
        words.insert(0, instruction("bra", sam_word_t(nwords)))
        words.append(instruction("ket", sam_word_t(nwords)))
        labels.update(sub_labels)
        return words, labels, nwords + 2

    toks: list[str] = inst.split(" ")
    inst = toks[0]
    operand = None

    if inst in ("int", "link", "float", "trap", "push"):
        if len(toks) != 2:
            raise ValueError(f"{inst} needs an operand")
        operand = toks[1]

        if inst in ("int", "link"):
            return [instruction(inst, sam_word_t(int(operand)))], labels, 1
        elif inst == "float":
            f = int(struct.pack("!f", float(operand)).hex())
            return (
                [
                    instruction("float", sam_word_t(f >> SAM_OP_SHIFT.value)),
                    instruction("_float", sam_word_t(f & SAM_OP_MASK.value)),
                ],
                labels,
                2,
            )
        elif inst == "trap":
            return [instruction("trap", sam_word_t(traps[operand]))], labels, 1
        else:  # inst == "push"
            operand = sam_word_t(int(operand))
            return (
                [
                    instruction(
                        "push", sam_word_t(operand.value >> SAM_OP_SHIFT.value)
                    ),
                    instruction("_push", sam_word_t(operand.value & SAM_OP_MASK.value)),
                ],
                labels,
                2,
            )
    else:
        if len(toks) != 1:
            raise ValueError(f"unexpected operand for {inst}")
        return [instruction(inst)], labels, 1


def compile_list(
    insts, pc0=sam_uword_t(0)
) -> tuple[list[sam_word_t], dict[str, sam_uword_t], int]:
    labels, words = {}, []
    pc = pc0
    for inst in insts:
        sub_words, sub_labels, n_sub_words = compile_inst(inst, pc)
        words.extend(sub_words)
        labels.update(sub_labels)
        pc = sam_uword_t(pc.value + n_sub_words)
    return words, labels, pc.value - pc0.value


def compile(program_file: str) -> tuple[list[sam_word_t], int]:
    prog = yaml.full_load(open(program_file).read())

    c_source = []
    words, labels, n_words = compile_list(prog)

    for label, n in labels.items():
        c_source.append(f"#define {label} {n}")
    for word in words:
        c_source.append(word)

    c_source.append(instruction("link"))
    c_source.append(instruction("link"))

    return c_source, n_words + 2
