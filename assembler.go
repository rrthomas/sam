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
	"math"
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
type assembler struct {
	code   []libsam.Word
	labels map[string]int
}

func (a *assembler) assemble(opcode libsam.Word, operand libsam.Word) {
	a.code = append(a.code, opcode|(operand<<libsam.OP_SHIFT))
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
	address, ok := a.labels[argStr]
	if ok {
		return libsam.Word(address)
	}
	if opcode, ok := parseInsn(argStr); ok {
		return libsam.Word(opcode)
	}
	if number, err := strconv.ParseInt(argStr, 10, 32); err == nil {
		return libsam.Word(number)
	}
	panic(fmt.Errorf("bad literal %s", argStr))
}

var operandInsns = []libsam.Word{
	libsam.INSN_INT,
	libsam.INSN_LINK,
	libsam.INSN_FLOAT,
	libsam.INSN_TRAP,
	libsam.INSN_PUSH,
}

func (a *assembler) assembleInstruction(tokens []string) {
	instStr := tokens[0]
	opcode, ok := parseInsn(instStr)
	if !ok {
		panic(fmt.Errorf("unknown instruction %s", instStr))
	}
	if slices.Contains(operandInsns, opcode) {
		if len(tokens) != 2 {
			panic(fmt.Errorf("%s needs an operand", instStr))
		}
		operandStr := tokens[1]

		switch opcode {
		case libsam.INSN_INT, libsam.INSN_LINK:
			operand := a.parseLiteral(operandStr)
			a.assemble(opcode, operand)
		case libsam.INSN_FLOAT:
			float, err := strconv.ParseFloat(operandStr, 32)
			if err != nil {
				panic(fmt.Errorf("bad float %s", operandStr))
			}
			operand := libsam.Word(math.Float32bits(float32(float)))
			a.assemble(libsam.INSN_FLOAT, operand>>libsam.OP_SHIFT)
			a.assemble(libsam.INSN__FLOAT, operand&libsam.OP_MASK)
		case libsam.INSN_TRAP:
			trap, ok := parseTrap(operandStr)
			if !ok {
				panic(fmt.Errorf("unknown trap %s", operandStr))
			}
			a.assemble(opcode, trap)
		case libsam.INSN_PUSH:
			operand := a.parseLiteral(operandStr)
			a.assemble(libsam.INSN_PUSH, operand>>libsam.OP_SHIFT)
			a.assemble(libsam.INSN__PUSH, operand&libsam.OP_MASK)
		}
	} else {
		if len(tokens) > 1 {
			panic(fmt.Errorf("unexpected operand for %s", instStr))
		}
		a.assemble(libsam.Word(opcode), 0)
	}
}

func (a *assembler) Visit(n ast.Node) ast.Visitor {
	switch n.Type() {
	case ast.SequenceType:
		seq := n.(ast.ArrayNode)
		// sub-routine
		i := seq.ArrayRange()
		pc0 := len(a.code)
		a.assemble(libsam.INSN_BRA, 0) // placeholder
		for i.Next() {
			ast.Walk(a, i.Value())
		}
		pc := len(a.code)
		len := pc - pc0 - 1
		a.assemble(libsam.INSN_KET, libsam.Word(len))
		a.code[pc0] |= libsam.Word(len << libsam.OP_SHIFT)
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
		a.labels[label] = len(a.code)
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
		ast.Walk(a, subProg)
		return nil
	default:
		panic(fmt.Errorf("invalid code %v", n))
	}
}

func Assemble(progFile string) []libsam.Word {
	prog := readProg(progFile)
	a := assembler{labels: map[string]int{}}
	ast.Walk(&a, prog)
	a.assemble(libsam.INSN_LINK, 1)
	a.assemble(libsam.INSN_LINK, 1)
	return a.code
}
