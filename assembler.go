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
	"bytes"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"strings"

	"github.com/goccy/go-yaml"
	"github.com/goccy/go-yaml/ast"
	"github.com/rrthomas/sam/libsam"
)

// Read the program from a YAML slice
func readProg(r io.Reader) ast.Node {
	dec := yaml.NewDecoder(r)

	var doc ast.Node
	if err := dec.Decode(&doc); err == io.EOF {
		panic(errors.New("input was empty"))
	}

	// Check that there was only one document in the file
	var eofDoc ast.Node
	if err := dec.Decode(&eofDoc); err != io.EOF {
		panic(errors.New("only one program allowed at a time"))
	}

	return doc
}

// Assemble a program
var labels map[string]address

type address struct {
	stack libsam.Stack
	item  libsam.Uword
}

type assembler struct {
	stack  libsam.Stack
	insts  libsam.Uword
	nInsts uint
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

func parseInsn(instStr string) (libsam.Instruction, bool) {
	insn, ok := libsam.Instructions[strings.ToLower(instStr)]
	return insn, ok
}

func parseTrap(trapStr string) (libsam.Word, bool) {
	trap, ok := libsam.Traps[strings.ToUpper(trapStr)]
	return libsam.Word(trap), ok
}

func (a *assembler) parseLiteral(argStr string) libsam.Word {
	if opcode, ok := parseTrap(argStr); ok {
		return libsam.Word(opcode)
	}
	if number, err := strconv.ParseInt(argStr, 10, libsam.WORD_BIT); err == nil {
		return libsam.Word(number)
	}
	panic(fmt.Errorf("bad literal %s", argStr))
}

func (a *assembler) parseLabel(argStr string) address {
	address, ok := labels[argStr]
	if ok {
		return address
	}
	panic(fmt.Errorf("no such label ‘%s’", argStr))
}

func (a *assembler) parseStack(argStr string) libsam.Stack {
	address := a.parseLabel(argStr)
	if address.item != 0 {
		panic(fmt.Errorf("expected stack but found other label ‘%s’", argStr))
	}
	return address.stack
}

func (a *assembler) parseString(argStr string) libsam.Stack {
	return libsam.NewString(argStr)
}

func (a *assembler) assembleInstruction(str string) {
	tokens := strings.Fields(str)
	if len(tokens) < 1 {
		panic(errors.New("empty instruction"))
	}
	instStr := tokens[0]
	insn, ok := parseInsn(instStr)
	if !ok {
		panic(fmt.Errorf("unknown instruction %s", instStr))
	}
	if insn.Operands != 0 {
		a.flushInstructions()

		if insn.Operands != -1 && len(tokens)-1 != insn.Operands {
			panic(fmt.Errorf("%s needs an operand", instStr))
		}
		operandStr := tokens[1]

		switch insn.Tag {
		case libsam.INT_TAG:
			a.stack.PushInt(libsam.Uword(a.parseLiteral(operandStr)))
		case libsam.BLOB_TAG:
			switch insn.Opcode {
			case libsam.BLOB_STACK:
				a.stack.PushArray(a.parseStack(operandStr))
			case libsam.BLOB_STRING:
				a.stack.PushArray(a.parseString(strings.TrimPrefix(strings.TrimSuffix(strings.TrimSpace(strings.TrimPrefix(strings.TrimSpace(str), "string")), "\""), "\"")))
			default:
				panic(fmt.Errorf("invalid blob type %+v", insn.Opcode))
			}
		case libsam.TRAP_TAG:
			trap, ok := parseTrap(operandStr)
			if !ok {
				panic(fmt.Errorf("unknown trap %s", operandStr))
			}
			a.stack.PushTrap(libsam.Uword(trap))
		case libsam.FLOAT_TAG:
			float, err := strconv.ParseFloat(operandStr, 64)
			if err != nil {
				panic(fmt.Errorf("bad float %s", operandStr))
			}
			a.stack.PushFloat(float64(float))
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
				a.stack.PushAtom(libsam.ATOM_NULL, 0)
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

func (a *assembler) assembleSequence(n ast.Node) {
	if n.Type() != ast.SequenceType {
		panic("program must be a list of instructions")
	}
	seq := n.(ast.ArrayNode)
	i := seq.ArrayRange()
	for i.Next() {
		ast.Walk(a, i.Value())
	}
	a.flushInstructions()
}

func (a *assembler) Visit(n ast.Node) ast.Visitor {
	switch n.Type() {
	case ast.SequenceType:
		a.flushInstructions()
		subA := assembler{stack: libsam.NewStack()}
		subA.assembleSequence(n)
		a.stack.PushArray(subA.stack)
		return nil
	case ast.StringType:
		a.assembleInstruction(n.String())
		return nil
	case ast.NullType: // Special case for "null"
		a.assembleInstruction("null")
		return nil
	case ast.MappingType:
		mapn := n.(*ast.MappingNode)
		vals := mapn.Values
		if len(vals) != 1 {
			panic(errors.New("bad label"))
		}
		mapNode := vals[0]
		keyNode := mapNode.Key
		if keyNode.Type() != ast.StringType {
			panic(fmt.Errorf("bad label: string expected"))
		}
		label := keyNode.String()
		a.flushInstructions()
		labels[label] = address{stack: a.stack, item: a.stack.Sp()}
		subNode := mapNode.Value
		ast.Walk(a, subNode)
		return nil
	case ast.TagType:
		tag := n.(*ast.TagNode)
		switch tagName := tag.GetToken().Value; tagName {
		case "!include":
			val := tag.Value
			if val.Type() != ast.StringType {
				panic(fmt.Errorf("invalid !include: string argument expected"))
			}
			file := val.String()
			r, err := os.Open(file)
			if err != nil {
				panic(err)
			}
			subProg := readProg(r)
			// Assemble the included file in a nested stack.
			a.flushInstructions()
			subA := assembler{stack: libsam.NewStack()}
			subA.assembleSequence(subProg)
			a.stack.PushArray(subA.stack)
		case "!istack":
			val := tag.Value
			if val.Type() != ast.StringType {
				panic(fmt.Errorf("invalid !istack: label argument expected"))
			}
			label := val.String()
			address := a.parseLabel(label)
			a.assembleInstruction(fmt.Sprintf("int %d", address.item))
			a.stack.PushArray(address.stack)
		case "!single":
			val := tag.Value
			if val.Type() != ast.StringType {
				panic(fmt.Errorf("invalid !single: instruction expected"))
			}
			instName := val.String()
			a.flushInstructions()
			a.assembleInstruction(instName)
			a.flushInstructions()
		default:
			panic(fmt.Errorf("invalid directive %s", tagName))
		}
		return nil
	default:
		panic(fmt.Errorf("invalid code %v", n))
	}
}

func Assemble(source []byte) libsam.Stack {
	prog := readProg(bytes.NewReader(source))
	labels = map[string]address{}
	a := assembler{stack: libsam.NewStack()}
	a.assembleSequence(prog)
	return a.stack
}
