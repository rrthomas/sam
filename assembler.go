/*
SAM assembler

Copyright © 2025-2026 Reuben Thomas <rrt@sc3d.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
package main

import (
	"errors"
	"fmt"
	"strconv"
	"strings"

	"github.com/rrthomas/sam/libsam"
)

// Assemble a program
func parseInsn(instStr string) libsam.Instruction {
	insn, ok := libsam.Instructions[strings.ToLower(instStr)]
	if !ok {
		panic(fmt.Errorf("unknown instruction %s", instStr))
	}
	return insn
}

func parseTrap(trapStr string) (libsam.Uword, bool) {
	trap, ok := libsam.Traps[strings.ToUpper(trapStr)]
	return libsam.Uword(trap), ok
}

func parseLiteral(argStr string) libsam.Word {
	if address, ok := labels[argStr]; ok {
		return libsam.Word(address.item)
	}
	if opcode, ok := parseTrap(argStr); ok {
		return libsam.Word(opcode)
	}
	if number, err := strconv.ParseInt(argStr, 10, libsam.WORD_BIT); err == nil {
		return libsam.Word(number)
	}
	panic(fmt.Errorf("bad literal %s", argStr))
}

type address struct {
	array libsam.Blob
	item  libsam.Uword
}

var labels map[string]address

func (a *assembler) newLabel(label string) {
	labels[label] = address{array: a.array, item: a.array.Sp()}
}

func (a *assembler) getLabel(label string) address {
	address, ok := labels[label]
	if ok {
		return address
	}
	panic(fmt.Errorf("no such label ‘%s’", label))
}

func (a *assembler) parseArray(argStr string) libsam.Blob {
	address := a.getLabel(argStr)
	if address.item != 0 {
		panic(fmt.Errorf("expected array but found other label ‘%s’", argStr))
	}
	return address.array
}

func (a *assembler) assembleInstruction(str string) {
	tokens := strings.Fields(str)
	if len(tokens) < 1 {
		panic(errors.New("empty instruction"))
	}
	instStr := tokens[0]
	insn := parseInsn(instStr)
	if insn.Operands != 0 {
		a.flushInstructions()

		if insn.Operands != -1 && len(tokens)-1 != insn.Operands {
			panic(fmt.Errorf("%s needs an operand", instStr))
		}
		operandStr := tokens[1]

		switch insn.Tag {
		case libsam.ATOM_TAG:
			a.flushInstructions()

			switch insn.Opcode {
			case libsam.ATOM_BOOL:
				switch operandStr {
				case "false":
					a.addBool(false)
				case "true":
					a.addBool(true)
				default:
					panic(fmt.Errorf("invalid boolean %s", operandStr))
				}
			default:
				panic(fmt.Errorf("invalid atom type %d", insn.Opcode))
			}
		case libsam.INT_TAG:
			a.addInt(parseLiteral(operandStr))
		case libsam.BLOB_TAG:
			switch insn.Opcode {
			case libsam.BLOB_ARRAY:
				a.addBlob(a.parseArray(operandStr))
			case libsam.BLOB_STRING:
				a.addBlob(libsam.NewString(strings.TrimPrefix(strings.TrimSuffix(strings.TrimSpace(strings.TrimPrefix(strings.TrimSpace(str), "string")), "\""), "\"")))
			default:
				panic(fmt.Errorf("invalid blob type %+v", insn.Opcode))
			}
		case libsam.TRAP_TAG:
			trap, ok := parseTrap(operandStr)
			if !ok {
				panic(fmt.Errorf("unknown trap %s", operandStr))
			}
			a.addTrap(trap)
		case libsam.FLOAT_TAG:
			float, err := strconv.ParseFloat(operandStr, 64)
			if err != nil {
				panic(fmt.Errorf("bad float %s", operandStr))
			}
			a.addFloat(float64(float))
		default:
			panic(fmt.Errorf("unknown instruction %+v", insn))
		}
	} else {
		if len(tokens) > 1 {
			panic(fmt.Errorf("unexpected operand for %s", instStr))
		}

		switch insn.Tag {
		case libsam.ATOM_TAG:
			a.flushInstructions()

			switch insn.Opcode {
			case libsam.ATOM_NULL:
				a.addNull()
			default:
				panic(fmt.Errorf("invalid atom type %d", insn.Opcode))
			}
		case libsam.INSTS_TAG:
			a.addInstruction(insn)
		default:
			panic(fmt.Errorf("unknown tag %d", insn.Tag))
		}
	}
}

func (a *assembler) assembleSingleInstruction(instStr string) {
	a.addSingleInstruction(parseInsn(instStr))
}
