/*
SAM code generator

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
	"fmt"

	"github.com/rrthomas/sam/libsam"
)

// Assemble a program
// FIXME: check all return codes from libsam and panic on error
var labels map[string]address

type address struct {
	stack libsam.Array
	item  libsam.Uword
}

type assembler struct {
	stack  libsam.Array
	insts  libsam.Uword
	nInsts uint
}

func (a *assembler) newLabel(label string) {
	labels[label] = address{stack: a.stack, item: a.stack.Sp()}
}

func (a *assembler) getLabel(label string) address {
	address, ok := labels[label]
	if ok {
		return address
	}
	panic(fmt.Errorf("no such label ‘%s’", label))
}

func (a *assembler) flushInstructions() {
	if a.nInsts > 0 {
		a.stack.PushInsts(a.insts)
	}
	a.nInsts = 0
	a.insts = 0
}

func (a *assembler) addInstruction(opcode libsam.Instruction) {
	if (a.nInsts+1)*uint(libsam.ONE_INST_SHIFT)+uint(libsam.INSTS_SHIFT) > uint(libsam.WORD_BIT) {
		a.flushInstructions()
	}
	a.insts |= (opcode.Opcode & libsam.Uword(libsam.INST_MASK)) << (libsam.Uword(a.nInsts) * libsam.Uword(libsam.ONE_INST_SHIFT))
	a.nInsts += 1
	if opcode.Terminal {
		a.flushInstructions()
	}
}

func (a *assembler) addTrap(function libsam.Uword) {
	a.flushInstructions()
	a.stack.PushTrap(function)
}

func (a *assembler) addNull() {
	a.flushInstructions()
	a.stack.PushAtom(libsam.ATOM_NULL, 0)
}

func (a *assembler) addBool(f bool) {
	a.flushInstructions()
	var boolVal libsam.Uword
	if f {
		boolVal = libsam.Uword(libsam.TRUE)
	} else {
		boolVal = libsam.Uword(libsam.FALSE)
	}
	a.stack.PushAtom(libsam.ATOM_BOOL, boolVal)
}

func (a *assembler) addInt(int libsam.Word) {
	a.flushInstructions()
	a.stack.PushInt(int)
}

func (a *assembler) addFloat(float float64) {
	a.flushInstructions()
	a.stack.PushFloat(float)
}

func (a *assembler) addStack(stack libsam.Array) {
	a.flushInstructions()
	a.stack.PushArray(stack)
}

func (a *assembler) addSingleInstruction(opcode libsam.Instruction) {
	a.flushInstructions()
	a.addInstruction(opcode)
	a.flushInstructions()
}
