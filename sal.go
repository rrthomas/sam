//nolint:govet
/*
SAL compiler

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
	"encoding/json"
	"fmt"
	"os"
	"slices"
	"strings"

	"github.com/alecthomas/participle/v2/lexer"
	"github.com/goccy/go-yaml"
	"github.com/rrthomas/sam/libsam"
)

// Parser

type PrimaryExp struct {
	Pos lexer.Position

	Int   *int64   `  @Int`
	Float *float64 `| @Float`
	// String
	// List
	// Map
	Block    *Block      `| @@`
	Function *Function   `| @@`
	Variable *string     `| @Ident`
	Paren    *Expression `| "(" @@ ")"`
}

type CallExp struct {
	Pos lexer.Position

	Function *PrimaryExp `@@`
	Calls    *[]Args     `@@*`
}

type Args struct {
	Pos lexer.Position

	Arguments *[]Expression `"(" (@@ ("," @@)* ","? )? ")"`
}

type UnaryExp struct {
	Pos lexer.Position

	Op         string    `  ( "~" | "+" | "-"`
	UnaryExp   *UnaryExp `  @@ )`
	PostfixExp *CallExp  `| @@`
}

type ExponentExp struct {
	Pos lexer.Position

	Left  *UnaryExp    `@@`
	Right *ExponentExp `[ "**" @@ ]`
}

type ProductExp struct {
	Pos lexer.Position

	Left  *ExponentExp `@@`
	Op    string       `[ @("*" | "/" | "%")`
	Right *ProductExp  `@@ ]`
}

type SumExp struct {
	Pos lexer.Position

	Left  *ProductExp `@@`
	Op    string      `[ @("+" | "-")`
	Right *SumExp     `@@ ]`
}

type CompareExp struct {
	Pos lexer.Position

	Left  *SumExp     `@@`
	Op    string      `[ @("==" | "!=" | "<" | "<=" | ">" | ">=")`
	Right *CompareExp `@@ ]`
}

type BitwiseExp struct {
	Pos lexer.Position

	Left  *CompareExp `@@`
	Op    string      `[ @("&" | "^" | "|" | "<<" | ">>" | ">>>")`
	Right *BitwiseExp `@@ ]`
}

type LogicNotExp struct {
	Pos lexer.Position

	LogicNotExp *LogicNotExp `  ( "not" @@ )`
	BitwiseExp  *BitwiseExp  `| @@`
}

type LogicExp struct {
	Pos lexer.Position

	Left  *LogicNotExp `@@`
	Op    string       `[ @("and" | "or")`
	Right *LogicExp    `@@ ]`
}

type Expression struct {
	Pos lexer.Position

	If       *If       `  @@`
	Loop     *Block    `| "loop" @@`
	LogicExp *LogicExp `| @@`
}

type If struct {
	Pos lexer.Position

	Cond *Expression `"if" @@`
	Then *Block      `@@`
	Else *Block      `("else" @@)?`
}

type Assignment struct {
	Pos lexer.Position

	Variable   *string     `@Ident "="`
	Expression *Expression `@@`
}

type Trap struct {
	Pos lexer.Position

	Function  *string       `"trap" @Ident`
	Arguments *[]Expression `("," @@)*`
}

type Declaration struct {
	Pos lexer.Position

	Variable *string     `@Ident ":"`
	Value    *Expression `@@ ";"`
}

type Statement struct {
	Pos lexer.Position

	Assignment *Assignment `  @@ ";"`
	Trap       *Trap       `| @@ ";"`
	Expression *Expression `| @@ ";"`
}

type Terminator struct {
	Return   *Expression `  "return" @@ ";"`
	BreakExp *Expression `| "break" @@ ";"`
	Break    bool        `| @"break" ";"`
	Continue bool        `| @"continue" ";"`
}

type Body struct {
	Pos lexer.Position

	Declarations *[]Declaration `@@*`
	Statements   *[]Statement   `@@*`
	Terminator   *Terminator    `@@?`
}

type Block struct {
	Pos lexer.Position

	Body *Body `"{" @@ "}"`
}

type Function struct {
	Pos lexer.Position

	Parameters *[]string `"fn" "(" @Ident ("," @Ident)* ","? ")"`
	Body       *Block    `@@`
}

// Compilation

func (e *PrimaryExp) Compile(ctx *Frame) {
	if e.Int != nil {
		ctx.assemble(fmt.Sprintf("int %d", *e.Int))
	} else if e.Float != nil {
		ctx.assemble(fmt.Sprintf("float %g", *e.Float))
	} else if e.Variable != nil {
		ctx.compileVar(*e.Variable)
		ctx.assemble("get")
	} else if e.Block != nil {
		ctx.assemble("int 0") // value of block
		ctx.assemble(ctx.assembleBlock(e.Block).asm)
		ctx.assemble("do")
	} else if e.Function != nil {
		e.Function.Compile(ctx)
	} else if e.Paren != nil {
		e.Paren.Compile(ctx)
	} else {
		panic("invalid PrimaryExp")
	}
}

func (e *UnaryExp) Compile(ctx *Frame) {
	if e.UnaryExp != nil {
		panic("implement compilation of UnaryExp")
	} else if e.PostfixExp != nil {
		e.PostfixExp.Compile(ctx)
	} else {
		panic("invalid UnaryExp")
	}
}

func (e *CallExp) Compile(ctx *Frame) {
	// Argument lists
	if e.Calls != nil {
		for i := len(*e.Calls) - 1; i >= 0; i-- {
			(*e.Calls)[i].Compile(ctx)
		}
	}

	// Initial function
	e.Function.Compile(ctx)

	// Call the successive functions, adjusting sp after each
	if e.Calls != nil {
		for _, args := range *e.Calls {
			ctx.assemble("do")
			nargs := len(*args.Arguments)
			if nargs > 1 {
				ctx.adjustSp(-(nargs - 1))
			}
		}
	}
}

func (a *Args) Compile(ctx *Frame) {
	// Push arguments
	for _, a := range *a.Arguments {
		a.Compile(ctx)
	}
	// If there are no arguments, leave space for return value.
	if len(*a.Arguments) == 0 {
		ctx.assemble("int 0")
	}
}

func (e *ExponentExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		ctx.assemble("pow")
	}
}

func (e *ProductExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "*":
			ctx.assemble("mul")
		case "/":
			ctx.assemble("div")
		case "%":
			ctx.assemble("rem")
		default:
			panic(fmt.Errorf("unknown ProductExp.Op %s", e.Op))
		}
	}
}

func (e *SumExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "+":
			ctx.assemble("add")
		case "-":
			ctx.assemble("neg", "add")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *CompareExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "==":
			ctx.assemble("eq", "neg")
		case "!=":
			ctx.assemble("eq", "not", "neg")
		case "<":
			ctx.assemble("lt", "neg")
		case "<=":
			ctx.assemble("_two", "extract", "lt", "not", "neg")
		case ">":
			ctx.assemble("_two", "extract", "lt", "neg")
		case ">=":
			ctx.assemble("lt", "not", "neg")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *BitwiseExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "&":
			ctx.assemble("and")
		case "^":
			ctx.assemble("xor")
		case "|":
			ctx.assemble("or")
		case "<<":
			ctx.assembleTrap("lsh")
		case ">>":
			ctx.assembleTrap("rsh")
		case ">>>":
			ctx.assembleTrap("arsh")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *LogicNotExp) Compile(ctx *Frame) {
	if e.LogicNotExp != nil {
		e.LogicNotExp.Compile(ctx)
		ctx.assemble("neg", "not", "neg")
	} else if e.BitwiseExp != nil {
		e.BitwiseExp.Compile(ctx)
	} else {
		panic("invalid LogicNotExp")
	}

}

func (e *LogicExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "and":
			panic("implement compilation of LogicExp and")
		case "or":
			panic("implement compilation of LogicExp or")
		default:
			panic(fmt.Errorf("unknown LogicExp.Op %s", e.Op))
		}
	}
}

func (e *Expression) Compile(ctx *Frame) {
	if e.If != nil {
		e.If.Compile(ctx)
	} else if e.Loop != nil {
		ctx.assemble("int 0")
		blockCtx := e.Loop.Compile(ctx, true)
		for range blockCtx.sp - blockCtx.baseSp {
			blockCtx.assemble("pop")
		}
		blockCtx.assemble(fmt.Sprintf("stack %s", blockCtx.label))
		blockCtx.assemble("go")
		// Add loop label to start of block
		if len(blockCtx.asm) > 0 {
			blockCtx.asm[0] = map[string]any{blockCtx.label: blockCtx.asm[0]}
		}
		ctx.assemble(blockCtx.asm)
		ctx.assemble("do")
	} else if e.LogicExp != nil {
		e.LogicExp.Compile(ctx)
	} else {
		panic("invalid Expression")
	}
}

func (i *If) Compile(ctx *Frame) {
	ctx.assemble("int 0") // return value
	thenCtx := ctx.assembleBlock(i.Then)
	elseCtx := Frame{}
	if i.Else != nil {
		elseCtx = ctx.assembleBlock(i.Else)
	}
	i.Cond.Compile(ctx)
	ctx.assemble(thenCtx.asm)
	ctx.assemble(elseCtx.asm)
	ctx.assemble("if")
}

func (a *Assignment) Compile(ctx *Frame) {
	a.Expression.Compile(ctx)
	ctx.assemble("_one", "get")
	ctx.compileVar(*a.Variable)
	ctx.assemble("set")
}

func (t *Trap) Compile(ctx *Frame) {
	stackEffect := trapStackEffect(*t.Function)
	nargs := 0
	if t.Arguments != nil {
		nargs = len(*t.Arguments)
	}
	if stackEffect.In != libsam.Uword(nargs) {
		panic(fmt.Errorf("trap %s takes %d argument(s), but %d supplied",
			*t.Function, stackEffect.In, nargs,
		))
	}
	if stackEffect.Out > 1 {
		panic("Support for traps returning more than one value is not implemented yet")
	}
	if t.Arguments != nil {
		for _, a := range *t.Arguments {
			a.Compile(ctx)
		}
	}
	ctx.assembleTrap(*t.Function)
	// If the trap returns no values, return a dummy value
	if stackEffect.Out == 0 {
		ctx.assemble("int 0")
	}
}

func (d *Declaration) Compile(ctx *Frame) {
	ctx.labels = append(ctx.labels, Location{id: *d.Variable, addr: int(ctx.sp)})
	d.Value.Compile(ctx)
}

func (s *Statement) Compile(ctx *Frame) {
	if s.Expression != nil {
		s.Expression.Compile(ctx)
	} else if s.Assignment != nil {
		s.Assignment.Compile(ctx)
	} else if s.Trap != nil {
		s.Trap.Compile(ctx)
	} else {
		panic("invalid Statement")
	}
}

func (t *Terminator) Compile(ctx *Frame) {
	if t.Return != nil {
		t.Return.Compile(ctx)
		ctx.tearDownFrame()
	} else if t.BreakExp != nil || t.Break {
		if ctx.loop == nil {
			panic("'break' used outside a loop")
		}
		// Set return value
		if t.BreakExp != nil {
			t.BreakExp.Compile(ctx)
		} else {
			ctx.assemble("zero")
		}
		ctx.assemble(fmt.Sprintf("int %d", ctx.loop.baseSp-(ctx.sp+3)), "set")
		// Pop items down to loop start
		for range ctx.sp - ctx.loop.baseSp {
			ctx.assemble("pop")
		}
		ctx.assemble("zero", "while")
	} else if t.Continue {
		if ctx.loop == nil {
			panic("'continue' used outside a loop")
		}
		// Pop items down to loop start
		for range ctx.sp - ctx.loop.baseSp {
			ctx.assemble("pop")
		}
		ctx.assemble(fmt.Sprintf("stack %s", ctx.loop.label))
		ctx.assemble("go")
	} else {
		panic("invalid Terminator")
	}
}

func (b *Body) Compile(ctx *Frame) {
	if b.Declarations != nil {
		for _, d := range *b.Declarations {
			d.Compile(ctx)
		}
	}
	if b.Statements != nil {
		for i, s := range *b.Statements {
			s.Compile(ctx)
			if i < len(*b.Statements)-1 || b.Terminator != nil {
				ctx.assemble("pop")
			}
		}
	}
	if b.Terminator != nil {
		b.Terminator.Compile(ctx)
	}
}

func (b *Block) Compile(ctx *Frame, loop bool) Frame {
	baseSp := libsam.Word(int(ctx.sp) + 2)
	blockCtx := Frame{
		label:  newLabel(),
		labels: slices.Clone(ctx.labels),
		asm:    make([]any, 0),
		baseSp: baseSp,
		sp:     baseSp,
		nargs:  ctx.nargs,
		loop:   ctx.loop,
	}
	if loop {
		blockCtx.loop = &blockCtx
	}
	b.Body.Compile(&blockCtx)
	return blockCtx
}

func (f *Function) Compile(ctx *Frame) {
	// FIXME: captures
	nargs := libsam.Uword(len(*f.Parameters))
	innerCtx := Frame{
		labels: make([]Location, 0),
		asm:    make([]any, 0),
		baseSp: 0,
		sp:     0,
		nargs:  nargs,
	}
	for i, p := range *f.Parameters {
		innerCtx.labels = append(innerCtx.labels, Location{id: p, addr: i - int(nargs)})
	}
	blockCtx := f.Body.Compile(&innerCtx, false)
	if f.Body.Body.Terminator == nil {
		blockCtx.tearDownFrame()
	}
	ctx.assemble(blockCtx.asm)
}

type Location struct {
	id   string
	addr int // relative to base of stack frame
}

func (ctx *Frame) adjustSp(delta int) {
	// FIXME: check this doesn't go outside some limit.
	ctx.sp += libsam.Word(delta)
}

func (ctx *Frame) compileVar(id string) {
	for i := len(ctx.labels) - 1; i >= 0; i-- {
		l := ctx.labels[i]
		if l.id == id {
			ctx.assemble(fmt.Sprintf("int %d", int(l.addr)-int(ctx.sp)))
			return
		}
	}
	panic(fmt.Errorf("no such variable %s", id))
}

func (ctx *Frame) tearDownBlock() {
	if ctx.sp < ctx.baseSp {
		panic(fmt.Sprintf("frame sp %d is below baseSp %d\n", ctx.sp, ctx.baseSp))
	}
	// Set result
	ctx.assemble(fmt.Sprintf("int %d", -int(ctx.sp-ctx.baseSp+3)), "set")
	// Pop remaining stack items in this frame, except for return address
	for range ctx.sp - ctx.baseSp {
		ctx.assemble("pop")
	}
}

func (ctx *Frame) tearDownFrame() {
	// Set result
	ctx.assemble(fmt.Sprintf("int %d", -int(ctx.sp+libsam.Word(ctx.nargs))), "set")
	// Pop remaining stack items in this frame, except for return address
	if ctx.sp > 2 {
		for range ctx.sp - 2 {
			ctx.assemble("pop")
		}
	}
	// If we have some arguments still on the stack, need to get rid of them
	if ctx.nargs == 2 {
		// Extract one remaining argument
		ctx.assemble("int -3", "extract", "pop")
	} else if ctx.nargs > 2 {
		// Store the return `pc` over the third argument
		ctx.assemble(fmt.Sprintf("int %d", -int(ctx.nargs)), "set")
		// Store the return `pc0` over the second argument
		ctx.assemble(fmt.Sprintf("int %d", -int(ctx.nargs)), "set")
		// Pop arguments 4 onwards, if any
		for range ctx.nargs - 3 {
			ctx.assemble("pop")
		}
	}
}

var nextLabel uint = 0

func newLabel() string {
	nextLabel += 1
	return fmt.Sprintf("$%d", nextLabel)
}

// FIXME: Separate Block (no args) from Frame
type Frame struct {
	label  string
	labels []Location
	asm    []any
	baseSp libsam.Word
	sp     libsam.Word
	nargs  libsam.Uword // Number of arguments to this frame
	loop   *Frame       // Innermost loop, if any
}

func (ctx *Frame) assemble(insts ...any) {
	for _, i := range insts {
		switch inst := i.(type) {
		case string:
			instName := strings.Fields(inst)[0]
			if instName == "trap" {
				panic("use assembleTrap to assemble a trap instruction")
			}
			delta, ok := libsam.StackDifference[instName]
			if !ok {
				panic(fmt.Errorf("invalid instruction %s", instName))
			}
			ctx.adjustSp(delta)
		default: // Assume a block (array or map)
			ctx.adjustSp(1) // A stack pushes a `ref` instruction
		}
		ctx.asm = append(ctx.asm, i)
	}
}

func trapStackEffect(function string) libsam.StackEffect {
	stackEffect, ok := libsam.TrapStackEffect[strings.ToUpper(function)]
	if !ok {
		panic(fmt.Errorf("unknown trap %s", function))
	}
	return stackEffect
}

func (ctx *Frame) assembleTrap(function string) {
	ctx.asm = append(ctx.asm, fmt.Sprintf("trap %s", function))
	stackEffect := trapStackEffect(function)
	ctx.adjustSp(int(stackEffect.Out) - int(stackEffect.In))
}

func (ctx *Frame) assembleBlock(b *Block) Frame {
	blockCtx := b.Compile(ctx, false)
	if b.Body.Terminator == nil {
		blockCtx.tearDownBlock()
	}
	return blockCtx
}

func Sal(src string, ast bool, asm bool) []byte {
	body, err := parser.ParseString("", src)
	if err != nil {
		panic(fmt.Errorf("error in source %v", err))
	}

	if ast {
		if err := json.NewEncoder(os.Stdout).Encode(body); err != nil {
			panic("error encoding JSON for AST")
		}
	}

	// Wrap the top-level body in a Block and compile it
	block := Block{Pos: body.Pos, Body: body}
	ctx := Frame{labels: make([]Location, 0), asm: make([]any, 0)}
	ctx.assemble("int 0") // value of top level
	blockCtx := ctx.assembleBlock(&block)
	ctx.assemble(blockCtx.asm)
	ctx.assemble("do", "halt")

	yaml, err := yaml.Marshal(ctx.asm)
	if err != nil {
		panic("error encoding YAML for compilation output")
	}
	if asm {
		print(string(yaml))
	}
	return yaml
}
