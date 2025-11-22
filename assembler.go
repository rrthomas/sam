/*
Copyright © 2025 Reuben Thomas <rrt@sc3d.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
package main

import (
	"errors"
	"fmt"
	"io"
	"os"
	"slices"
	"strconv"
	"strings"

	"github.com/goccy/go-yaml"
	"github.com/goccy/go-yaml/ast"
	"github.com/rrthomas/sam/libsam"
)

func check(e error) {
	if e != nil {
		panic(e)
	}
}

// Read the program from a YAML file
func readProg(progFile string) ast.Node {
	r, err := os.Open(progFile)
	check(err)
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
type address struct {
	stack libsam.Stack
	pos   libsam.Uword
}

var labels map[string]address

// FIXME: Elide this type.
type assembler struct {
	stack libsam.Stack
}

func parseInsn(instStr string) (libsam.Word, bool) {
	opcode, ok := libsam.Instructions[strings.ToLower(instStr)]
	return libsam.Word(opcode), ok
}

func parseTrap(trapStr string) (libsam.Word, bool) {
	trap, ok := libsam.Traps[strings.ToUpper(trapStr)]
	return libsam.Word(trap), ok
}

func (a *assembler) parseLiteral(argStr string) libsam.Word {
	address, ok := labels[argStr]
	if ok {
		if address.stack != a.stack {
			panic(fmt.Errorf("cannot use label from another stack as literal: %s", argStr))
		}
		return libsam.Word(address.pos)
	}
	if opcode, ok := parseInsn(argStr); ok {
		return libsam.Word(opcode)
	}
	if number, err := strconv.ParseInt(argStr, 10, 32); err == nil {
		return libsam.Word(number)
	}
	panic(fmt.Errorf("bad literal %s", argStr))
}

func (a *assembler) parseAddress(argStr string) address {
	if address, ok := labels[argStr]; ok {
		return address
	}
	panic(fmt.Errorf("bad address %s", argStr))
}

var operandInsns = []string{
	"int",
	"float",
	"link",
	"code",
	"data",
	"trap",
	"push",
}

var pseudoInsns = []string{
	"code",
	"data",
}

func (a *assembler) assembleInstruction(tokens []string) {
	instStr := tokens[0]
	instName := strings.ToLower(instStr)
	opcode, ok := parseInsn(instName)
	pseudoInsn := false
	if !ok {
		if slices.Contains(pseudoInsns, instName) {
			pseudoInsn = true
		} else {
			panic(fmt.Errorf("unknown instruction %s", instStr))
		}
	}
	if slices.Contains(operandInsns, instName) {
		if len(tokens) != 2 {
			panic(fmt.Errorf("%s needs an operand", instStr))
		}
		operandStr := tokens[1]

		if pseudoInsn {
			switch instName {
			case "code":
				addr := a.parseAddress(operandStr)
				ok, insn := addr.stack.StackPeek(addr.pos)
				if ok != libsam.ERROR_OK {
					panic(fmt.Errorf("invalid address for label %s: %d", operandStr, addr.pos))
				}
				a.stack.PushStack(libsam.Word(insn))
			case "data":
				// FIXME
			default:
				panic(fmt.Errorf("invalid pseudo-instruction %s", instStr))
			}
		} else {
			switch opcode {
			case libsam.TAG_ATOM | (libsam.ATOM_INT << libsam.ATOM_TYPE_SHIFT):
				operand := a.parseLiteral(operandStr)
				a.stack.PushAtom(libsam.ATOM_INT, libsam.Uword(operand))
			case libsam.TAG_LINK:
				operand := a.parseLiteral(operandStr)
				a.stack.PushLink(libsam.Uword(operand))
			case libsam.TAG_ATOM | (libsam.ATOM_INST << libsam.ATOM_TYPE_SHIFT):
				trap, ok := parseTrap(operandStr)
				if !ok {
					panic(fmt.Errorf("unknown trap %s", operandStr))
				}
				a.stack.PushAtom(libsam.ATOM_INST, libsam.Uword(trap))
			case libsam.TAG_ATOM | (libsam.ATOM_FLOAT << libsam.ATOM_TYPE_SHIFT):
				float, err := strconv.ParseFloat(operandStr, 32)
				if err != nil {
					panic(fmt.Errorf("bad float %s", operandStr))
				}
				a.stack.PushFloat(float32(float))
			}
		}
	} else {
		if pseudoInsn {
			panic("unexpected pseudo-instruction")
		}
		if len(tokens) > 1 {
			panic(fmt.Errorf("unexpected operand for %s", instStr))
		}
		a.stack.PushAtom(libsam.ATOM_INST, libsam.Uword(opcode))
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
}

func (a *assembler) Visit(n ast.Node) ast.Visitor {
	switch n.Type() {
	case ast.SequenceType:
		subA := assembler{stack: libsam.NewStack()}
		a.stack.PushCode(subA.stack)
		subA.assembleSequence(n)
		return nil
	case ast.StringType:
		tokens := strings.Fields(n.String())
		if len(tokens) < 1 {
			panic(errors.New("empty instruction"))
		}
		a.assembleInstruction(tokens)
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
		labels[label] = address{stack: a.stack, pos: a.stack.Sp()}
		subNode := mapNode.Value
		ast.Walk(a, subNode)
		return nil
	case ast.TagType:
		tag := n.(*ast.TagNode)
		tagName := tag.GetToken().Value
		if tagName != "!include" {
			panic(fmt.Errorf("invalid directive %s", tagName))
		}
		name := tag.Value
		if name.Type() != ast.StringType {
			panic(fmt.Errorf("invalid directive: string argument expected"))
		}
		file := name.String()
		subProg := readProg(file)
		// Assemble the included file in a nested stack.
		subA := assembler{stack: libsam.NewStack()}
		a.stack.PushCode(subA.stack)
		subA.assembleSequence(subProg)
		return nil
	default:
		panic(fmt.Errorf("invalid code %v", n))
	}
}

func Assemble(progFile string) {
	prog := readProg(progFile)
	labels = map[string]address{}
	a := assembler{stack: libsam.SamStack}
	a.assembleSequence(prog)
}
