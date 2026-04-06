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
	"strconv"
	"strings"

	"github.com/alecthomas/participle/v2/lexer"
	"github.com/rrthomas/sam/libsam"
)

// Parser

type Pair struct {
	Pos lexer.Position

	Key   *Expression `@@ [":"`
	Value *Expression `@@ ]`
}

type Boolean bool

func (b *Boolean) Capture(values []string) error {
	*b = values[0] == "true"
	return nil
}

// String with escapes.
type String struct {
	Pos lexer.Position

	Fragments []*StringFragment `"\"" @@* "\""`
}

type StringFragment struct {
	Pos lexer.Position

	Escaped string `  @Escaped`
	String  string `| @Chars`
}

type PrimaryExp struct {
	Pos lexer.Position

	Null      bool        `  @"null"`
	Bool      *Boolean    `| @("false" | "true")`
	Int       *int64      `| @Int`
	Float     *float64    `| @Float`
	String    *String     `| @@`
	EmptyMap  bool        `| @"[" ":" "]"`
	Container *[]Pair     `| "[" [@@ ("," @@)* ","?] "]"`
	Block     *Block      `| @@`
	Function  *Function   `| @@`
	Variable  *string     `| @Ident`
	Paren     *Expression `| "(" @@ ")"`
}

type IndexedExp struct {
	Pos lexer.Position

	Object  *PrimaryExp   `@@`
	Indexes *[]Expression `("[" @@ "]")*`
}

type CallExp struct {
	Pos lexer.Position

	Function *IndexedExp `@@`
	Calls    *[]Args     `@@*`
}

type Args struct {
	Pos lexer.Position

	Arguments *[]Expression `"(" [@@ ("," @@)* ","?] ")"`
}

type UnaryExp struct {
	Pos lexer.Position

	PreOp          string    `  @("~" | "+" | "-" | "#" | "<<<")`
	PrefixUnaryExp *UnaryExp `  @@`
	PostfixExp     *CallExp  `| @@`
	PostOp         *string   `  [@">>>"]`
}

type ExponentExp struct {
	Pos lexer.Position

	Left  *UnaryExp    `@@`
	Right *ExponentExp `["**" @@]`
}

type ProductExp struct {
	Pos lexer.Position

	Left  *ExponentExp `@@`
	Op    string       `[@("*" | "/" | "%")`
	Right *ProductExp  `@@]`
}

type SumExp struct {
	Pos lexer.Position

	Left  *ProductExp `@@`
	Op    string      `[@("+" | "-")`
	Right *SumExp     `@@]`
}

type CompareExp struct {
	Pos lexer.Position

	Left  *SumExp     `@@`
	Op    string      `[@("==" | "!=" | "<" | "<=" | ">" | ">=")`
	Right *CompareExp `@@]`
}

type BitwiseExp struct {
	Pos lexer.Position

	Left  *CompareExp `@@`
	Op    string      `[@("&" | "^" | "|")`
	Right *BitwiseExp `@@]`
}

type PushExp struct {
	Pos lexer.Position

	Left  *BitwiseExp `@@`
	Op    string      `[@("<<" | ">>")`
	Right *PushExp    `@@]`
}

type LogicNotExp struct {
	Pos lexer.Position

	LogicNotExp *LogicNotExp `  "not" @@`
	PushExp     *PushExp     `| @@`
}

type LogicExp struct {
	Pos lexer.Position

	Left  *LogicNotExp `@@`
	Op    string       `[@("and" | "or")`
	Right *LogicExp    `@@]`
}

type Expression struct {
	Pos lexer.Position

	Ifs        *Ifs        `  @@`
	Loop       *Block      `| "loop" @@`
	ForVar     string      `| "for" @Ident "in"`
	Iter       *Expression `  @@`
	Body       *Block      `  @@`
	Expression *LogicExp   `| @@`
}

type Ifs struct {
	Pos lexer.Position

	IfList    *[]If  `@@ ("else" @@)*`
	FinalElse *Block `["else" @@]`
}

type If struct {
	Pos lexer.Position

	Cond *Expression `"if" @@`
	Then *Block      `@@`
}

type Declaration struct {
	Pos lexer.Position

	Variable *string     `"let" @Ident "="`
	Value    *Expression `@@`
}

type Assignment struct {
	Pos lexer.Position

	Lvalue     *Expression `@@ [":="`
	Expression *Expression `@@]`
}

type Statement struct {
	Pos lexer.Position

	Empty        bool           `  @";"`
	Assignment   *Assignment    `| @@ ";"`
	Declarations *[]Declaration `| (@@ ";")+`
}

type Terminator struct {
	Return   *Expression `  "return" @@ ";"`
	BreakExp *Expression `| "break" @@ ";"`
	Break    bool        `| @"break" ";"`
	Continue bool        `| @"continue" ";"`
}

type Body struct {
	Pos lexer.Position

	Statements *[]Statement `@@*`
	Terminator *Terminator  `@@?`
}

type Block struct {
	Pos lexer.Position

	Body *Body `"{" @@ "}"`
}

type Function struct {
	Pos lexer.Position

	Parameters *[]string `"fn" "(" [@Ident ("," @Ident)* ","?] ")"`
	Body       *Block    `@@`
}

// Compilation

func (e *PrimaryExp) Compile(ctx *Frame) {
	if e.Null {
		ctx.assembleNull()
	} else if e.Bool != nil {
		if *e.Bool {
			ctx.assembleInst("true")
		} else {
			ctx.assembleInst("false")
		}
	} else if e.Int != nil {
		ctx.assembleInt(int(*e.Int))
	} else if e.Float != nil {
		ctx.assembleFloat(*e.Float)
	} else if e.String != nil {
		str := "`"
		for _, s := range e.String.Fragments {
			str += s.String + s.Escaped
		}
		str += "`"
		str, ok := strconv.Unquote(str)
		if ok != nil {
			panic(fmt.Errorf("Invalid string %s", str))
		}
		ctx.assembleBlob(libsam.NewString(str))
	} else if e.Container != nil {
		// Check we have a list or map, not a combination
		isMap := false
		for _, e := range *e.Container {
			if e.Value != nil {
				isMap = true
			} else if isMap == true {
				panic("bad list or map literal")
			}
		}
		if !isMap {
			ctx.assembleInst("new")
			for _, e := range *e.Container {
				e.Key.Compile(ctx)
				ctx.assembleInst("_two")
				ctx.assembleInst("s0")
				ctx.assembleInst("get")
				ctx.assembleInst("append")
			}
		} else {
			ctx.assembleTrap("new_map")
			for _, e := range *e.Container {
				e.Value.Compile(ctx)
				e.Key.Compile(ctx)
				ctx.assembleInt(-3)
				ctx.assembleInst("s0")
				ctx.assembleInst("get")
				ctx.assembleInst("set")
			}
		}
	} else if e.EmptyMap {
		ctx.assembleTrap("new_map")
	} else if e.Variable != nil {
		ctx.compileGetVar(*e.Variable)
	} else if e.Block != nil {
		ctx.assembleNull() // value of block
		ctx.assembleCode(ctx.assembleBlock(e.Block, false).asm)
		ctx.assembleInst("do")
	} else if e.Function != nil {
		e.Function.Compile(ctx)
	} else if e.Paren != nil {
		e.Paren.Compile(ctx)
	} else { // empty list
		ctx.assembleInst("new")
	}
}

func (ctx *Frame) compileIndexedExp(object *PrimaryExp, indexes *[]Expression, set bool) {
	if indexes != nil {
		n_indexes := len(*indexes)
		for i := range *indexes {
			(*indexes)[n_indexes-i-1].Compile(ctx)
		}
	}
	object.Compile(ctx)
	if indexes != nil {
		for range len(*indexes) - 1 {
			ctx.assembleInst("get")
		}
		if set {
			ctx.assembleInst("set")
		} else {
			ctx.assembleInst("get")
		}
	}

}

func (e *IndexedExp) Compile(ctx *Frame) {
	ctx.compileIndexedExp(e.Object, e.Indexes, false)
}

func (e *UnaryExp) Compile(ctx *Frame) {
	if e.PrefixUnaryExp != nil {
		e.PrefixUnaryExp.Compile(ctx)
		switch e.PreOp {
		case "~":
			ctx.assembleInst("not")
		case "+":
			break // no-op
		case "-":
			ctx.assembleInst("neg")
		case "#":
			ctx.assembleTrap("size")
		case "<<<":
			ctx.assembleInst("shift")
		default:
			panic(fmt.Errorf("unknown UnaryExp.PreOp %s", e.PreOp))
		}
	} else if e.PostfixExp != nil {
		e.PostfixExp.Compile(ctx)

		// Deal with PostOp, if any
		if e.PostOp != nil {
			switch *e.PostOp {
			case ">>>":
				ctx.assembleInst("pop")
			default:
				panic(fmt.Errorf("unknown UnaryExp.PostOp %s", *e.PostOp))
			}
		}
	} else {
		panic("invalid UnaryExp")
	}
}

func (e *CallExp) Compile(ctx *Frame) {
	// Check whether the first call is a trap
	haveTrap := false
	maybeId := e.Function.Object.Variable
	if maybeId != nil {
		haveTrap = ctx.isTrap(*maybeId)
	}

	// Argument lists
	if e.Calls != nil {
		for i := len(*e.Calls) - 1; i >= 0; i-- {
			if i == 0 && haveTrap {
				args := (*e.Calls)[i].Arguments
				if args != nil {
					for _, a := range *args {
						a.Compile(ctx)
					}
				}
			} else {
				(*e.Calls)[i].Compile(ctx)
			}
		}
	}

	// Initial function
	e.Function.Compile(ctx)

	// Call the successive functions, adjusting sp after each
	if e.Calls != nil {
		for i, args := range *e.Calls {
			if i == 0 && haveTrap {
				ctx.compileTrapCall(*e.Function.Object.Variable, args.Arguments)
			} else {
				ctx.assembleInst("new")
				ctx.assembleInst("call")
				if args.Arguments != nil {
					ctx.adjustSp(-(len(*args.Arguments)))
				}
			}
		}
	}
}

func (a *Args) Compile(ctx *Frame) {
	// Push arguments
	if a.Arguments == nil {
		ctx.assembleInst("zero")
		return
	}
	for _, a := range *a.Arguments {
		a.Compile(ctx)
	}
	// Push number of arguments
	ctx.assembleInt(len(*a.Arguments))
}

func (e *ExponentExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		ctx.assembleTrap("pow")
	}
}

func (e *ProductExp) Compile(ctx *Frame) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "*":
			ctx.assembleInst("mul")
		case "/":
			ctx.assembleTrap("div")
		case "%":
			ctx.assembleTrap("rem")
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
			ctx.assembleInst("add")
		case "-":
			ctx.assembleInst("neg")
			ctx.assembleInst("add")
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
			ctx.assembleInst("eq")
			ctx.assembleInst("neg")
		case "!=":
			ctx.assembleInst("eq")
			ctx.assembleInst("not")
			ctx.assembleInst("neg")
		case "<":
			ctx.assembleInst("lt")
			ctx.assembleInst("neg")
		case "<=":
			ctx.assembleInst("_two")
			ctx.assembleInst("s0")
			ctx.assembleInst("extract")
			ctx.assembleInst("lt")
			ctx.assembleInst("not")
			ctx.assembleInst("neg")
		case ">":
			ctx.assembleInst("_two")
			ctx.assembleInst("s0")
			ctx.assembleInst("extract")
			ctx.assembleInst("lt")
			ctx.assembleInst("neg")
		case ">=":
			ctx.assembleInst("lt")
			ctx.assembleInst("not")
			ctx.assembleInst("neg")
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
			ctx.assembleInst("and")
		case "^":
			ctx.assembleInst("xor")
		case "|":
			ctx.assembleInst("or")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *PushExp) Compile(ctx *Frame) {
	if e.Right != nil {
		switch e.Op {
		case "<<": // List append: l << i
			e.Left.Compile(ctx)
			e.Right.Compile(ctx)
			ctx.assembleInst("_two")
			ctx.assembleInst("s0")
			ctx.assembleInst("get")
			ctx.assembleInst("append")
		case ">>": // List prepend: i >> l
			e.Right.Compile(ctx)
			e.Left.Compile(ctx)
			ctx.assembleInst("_two")
			ctx.assembleInst("s0")
			ctx.assembleInst("get")
			ctx.assembleInst("prepend")
		default:
			panic(fmt.Errorf("unknown PushExp.Op %s", e.Op))
		}
	} else {
		e.Left.Compile(ctx)
	}
}

func (e *LogicNotExp) Compile(ctx *Frame) {
	if e.LogicNotExp != nil {
		e.LogicNotExp.Compile(ctx)
		ctx.assembleInst("neg")
		ctx.assembleInst("not")
		ctx.assembleInst("neg")
	} else if e.PushExp != nil {
		e.PushExp.Compile(ctx)
	} else {
		panic("invalid LogicNotExp")
	}

}

func (e *LogicExp) Compile(ctx *Frame) {
	if e.Right == nil {
		e.Left.Compile(ctx)
	} else {
		switch e.Op {
		case "and":
			ctx.assembleNull()
			thenCtx := ctx.newBlock(false)
			e.Right.Compile(&thenCtx)
			thenCtx.tearDownBlock()
			elseCtx := ctx.newBlock(false)
			elseCtx.assembleInst("false")
			elseCtx.tearDownBlock()
			e.Left.Compile(ctx)
			ctx.assembleCode(thenCtx.asm)
			ctx.assembleCode(elseCtx.asm)
			ctx.assembleInst("if")
		case "or":
			ctx.assembleNull()
			elseCtx := ctx.newBlock(false)
			e.Right.Compile(&elseCtx)
			elseCtx.tearDownBlock()
			thenCtx := ctx.newBlock(false)
			thenCtx.assembleInst("true")
			thenCtx.tearDownBlock()
			e.Left.Compile(ctx)
			ctx.assembleCode(thenCtx.asm)
			ctx.assembleCode(elseCtx.asm)
			ctx.assembleInst("if")
		default:
			panic(fmt.Errorf("unknown LogicExp.Op %s", e.Op))
		}
	}
}

func (e *Expression) Compile(ctx *Frame) {
	if e.Ifs != nil {
		e.Ifs.Compile(ctx)
	} else if e.Loop != nil {
		ctx.assembleNull()
		blockCtx := e.Loop.Compile(ctx, true)
		ctx.assembleLoop(&blockCtx)
	} else if e.ForVar != "" {
		// Initialize iterator
		ctx.locals = append(ctx.locals, Local{id: "$iter", pos: int(ctx.sp)})
		e.Iter.Compile(ctx)
		ctx.assembleTrap("iter")
		// Loop body
		ctx.assembleNull()
		blockCtx := ctx.newBlock(true)
		// Call iterator and test for termination
		blockCtx.locals = append(blockCtx.locals, Local{id: e.ForVar, pos: int(blockCtx.sp)})
		blockCtx.compileGetVar("$iter")
		blockCtx.assembleTrap("next")
		blockCtx.assembleNull() // return value
		thenCtx := blockCtx.newBlock(false)
		thenCtx.assembleBreak()
		elseCtx := blockCtx.assembleBlock(e.Body, false)
		blockCtx.assembleInst("_two")
		blockCtx.assembleInst("s0")
		blockCtx.assembleInst("get")
		blockCtx.assembleNull()
		blockCtx.assembleInst("eq")
		blockCtx.assembleCode(thenCtx.asm)
		blockCtx.assembleCode(elseCtx.asm)
		blockCtx.assembleInst("if")
		// Complete the loop body and add to the current context
		ctx.assembleLoop(&blockCtx)
	} else if e.Expression != nil {
		e.Expression.Compile(ctx)
	} else {
		panic("invalid Expression")
	}
}

func (ctx *Frame) compileIfs(il *[]If, fe *Block) {
	if il == nil || len(*il) == 0 {
		panic("unexpected nil or empty IfList")
	}
	ctx.assembleNull() // return value
	thenCtx := ctx.assembleBlock((*il)[0].Then, false)
	elseCtx := Frame{asm: &assembler{stack: libsam.NewArray()}}
	if len(*il) > 1 {
		elseCtx = ctx.newBlock(false)
		restIl := (*il)[1:]
		restIfs := Ifs{Pos: (*il)[1].Pos, IfList: &restIl, FinalElse: fe}
		restIfs.Compile(&elseCtx)
		elseCtx.tearDownBlock()
	} else if fe != nil {
		elseCtx = ctx.assembleBlock(fe, false)
	}
	(*il)[0].Cond.Compile(ctx)
	ctx.assembleCode(thenCtx.asm)
	ctx.assembleCode(elseCtx.asm)
	ctx.assembleInst("if")
}

func (i *Ifs) Compile(ctx *Frame) {
	ctx.compileIfs(i.IfList, i.FinalElse)
}

// Assignment, or an rvalue that is syntactically an lvalue
func (a *Assignment) Compile(ctx *Frame) {
	if a.Expression != nil {
		a.Expression.Compile(ctx)
		ctx.assembleInst("_one")
		ctx.assembleInst("s0")
		ctx.assembleInst("get")
		ctx.compileAssignLvalue(a.Lvalue)
	} else {
		a.Lvalue.Compile(ctx)
	}
}

func (ctx *Frame) compileTrapCall(function string, args *[]Expression) {
	stackEffect := trapStackEffect(function)
	nargs := 0
	if args != nil {
		nargs = len(*args)
	}
	if stackEffect.In != libsam.Uword(nargs) {
		panic(fmt.Errorf("trap %s takes %d argument(s), but %d supplied",
			function, stackEffect.In, nargs,
		))
	}
	if stackEffect.Out > 1 {
		panic("Support for traps returning more than one value is not implemented yet")
	}
	// If the trap returns no values, return a dummy value
	if stackEffect.Out == 0 {
		ctx.assembleNull()
	}
}

func (d *Declaration) Compile(ctx *Frame) {
	ctx.locals = append(ctx.locals, Local{id: *d.Variable, pos: int(ctx.sp)})
	d.Value.Compile(ctx)
}

func (s *Statement) Compile(ctx *Frame) {
	if s.Empty {
		// Do nothing
	} else if s.Assignment != nil {
		s.Assignment.Compile(ctx)
	} else if s.Declarations != nil {
		for _, d := range *s.Declarations {
			d.Compile(ctx)
		}
	} else {
		panic("invalid Statement")
	}
}

func (t *Terminator) Compile(ctx *Frame) {
	if t.Return != nil {
		t.Return.Compile(ctx)
		ctx.assembleReturn()
	} else if t.BreakExp != nil || t.Break {
		if ctx.loop == nil {
			panic("'break' used outside a loop")
		}
		// Set return value
		if t.BreakExp != nil {
			t.BreakExp.Compile(ctx)
		} else {
			ctx.assembleNull()
		}
		ctx.assembleInt(int(ctx.loop.baseSp - (ctx.sp + 3)))
		ctx.assembleInst("s0")
		ctx.assembleInst("set")
		ctx.assembleBreak()
	} else if t.Continue {
		if ctx.loop == nil {
			panic("'continue' used outside a loop")
		}
		// Drop items down to loop start
		for range ctx.sp - ctx.loop.baseSp {
			ctx.assembleInst("drop")
		}
		ctx.assembleCode(ctx.loop.asm)
		ctx.assembleInst("go")
	} else {
		panic("invalid Terminator")
	}
}

func (b *Body) Compile(ctx *Frame) {
	if b.Statements != nil {
		for i, s := range *b.Statements {
			s.Compile(ctx)
			if s.Declarations == nil && (i < len(*b.Statements)-1 || b.Terminator != nil) {
				ctx.assembleInst("drop")
			}
		}
	}
	if b.Terminator != nil {
		b.Terminator.Compile(ctx)
	}
}

func (b *Block) Compile(ctx *Frame, loop bool) Frame {
	blockCtx := ctx.newBlock(loop)
	b.Body.Compile(&blockCtx)
	return blockCtx
}

func (f *Function) Compile(ctx *Frame) {
	nargs := libsam.Uword(0)
	if f.Parameters != nil {
		nargs = libsam.Uword(len(*f.Parameters))
	}
	captures := make([]Capture, 0)
	innerCtx := Frame{
		parent:   ctx,
		locals:   make([]Local, 0),
		captures: &captures,
		asm:      &assembler{stack: libsam.NewArray()},
		baseSp:   0,
		sp:       libsam.Word(nargs) + 1, // exclude P0 from count
		nargs:    nargs,
	}
	if f.Parameters != nil {
		for i, p := range *f.Parameters {
			innerCtx.locals = append(innerCtx.locals, Local{id: p, pos: i})
		}
	}
	ctx.assembleInst("new") // closure array
	ctx.assembleInst("new") // captures array
	blockCtx := f.Body.Compile(&innerCtx, false)
	ctx.assembleInst("_two") // append captures to closure
	ctx.assembleInst("s0")
	ctx.assembleInst("get")
	ctx.assembleInst("append")
	if f.Body.Body.Terminator == nil {
		blockCtx.assembleReturn()
	}
	ctx.assembleCode(blockCtx.asm)
	ctx.assembleInst("_two") // append code to closure
	ctx.assembleInst("s0")
	ctx.assembleInst("get")
	ctx.assembleInst("append")
	ctx.assembleQuote("go")
	ctx.assembleInst("_two") // append tail call to closure
	ctx.assembleInst("s0")
	ctx.assembleInst("get")
	ctx.assembleInst("append")
}

type Local struct {
	id  string
	pos int // relative to base of stack frame
}

type Capture struct {
	id string
}

func (ctx *Frame) adjustSp(delta int) {
	// FIXME: check this doesn't go outside some limit.
	ctx.sp += libsam.Word(delta)
}

func (ctx *Frame) findLocal(id string) *Local {
	for i := len(ctx.locals) - 1; i >= 0; i-- {
		l := ctx.locals[i]
		if l.id == id {
			return &l
		}
	}
	return nil
}

func (ctx *Frame) findCapture(id string) *uint {
	for i := len(*ctx.captures) - 1; i >= 0; i-- {
		c := (*ctx.captures)[i]
		if c.id == id {
			index := uint(i)
			return &index
		}
	}
	if parent := ctx.parent; parent != nil {
		found := false
		if l := parent.findLocal(id); l != nil {
			// Append address of local to captures array
			parent.assembleInst("s0")
			parent.assembleInst("_two")
			parent.assembleInst("s0")
			parent.assembleInst("get")
			parent.assembleInst("append")
			parent.compileLocalAddr(*l)
			parent.assembleInst("_two")
			parent.assembleInst("s0")
			parent.assembleInst("get")
			parent.assembleInst("append")
			found = true
		} else if i := parent.findCapture(id); i != nil {
			// Append address of capture to captures array
			parent.compileCaptureAddr(*i)
			parent.assembleInt(-3)
			parent.assembleInst("s0")
			parent.assembleInst("get")
			parent.assembleInst("append")
			parent.assembleInst("_two")
			parent.assembleInst("s0")
			parent.assembleInst("get")
			parent.assembleInst("append")
			found = true
		}
		if found {
			newCaptureIndex := uint(len(*ctx.captures))
			*ctx.captures = append(*ctx.captures, Capture{id: id})
			return &newCaptureIndex
		}
	}
	return nil
}

func (ctx *Frame) compileLocalAddr(l Local) {
	ctx.assembleInt(int(l.pos))
}

func (ctx *Frame) compileCaptureAddr(i uint) {
	ctx.assembleInt(int(ctx.nargs + 2))
	ctx.assembleInst("s0")
	ctx.assembleInst("get")
	ctx.assembleInt(int(i*2 + 1))
	ctx.assembleInst("_two")
	ctx.assembleInst("s0")
	ctx.assembleInst("get")
	ctx.assembleInst("get")
	ctx.assembleInt(int(i * 2))
	ctx.assembleInt(-3)
	ctx.assembleInst("s0")
	ctx.assembleInst("extract")
	ctx.assembleInst("get")
}

func (ctx *Frame) isTrap(id string) bool {
	if ctx.findLocal(id) == nil || ctx.findCapture(id) == nil {
		if _, ok := libsam.TrapStackEffect[strings.ToUpper(id)]; ok {
			return true
		}
	}
	return false
}

func (ctx *Frame) compileGetVar(id string) {
	if l := ctx.findLocal(id); l != nil {
		ctx.compileLocalAddr(*l)
		ctx.assembleInst("s0")
		ctx.assembleInst("get")
	} else if c := ctx.findCapture(id); c != nil {
		ctx.compileCaptureAddr(*c)
		ctx.assembleInst("get")
	} else if _, ok := libsam.TrapStackEffect[strings.ToUpper(id)]; ok {
		ctx.assembleTrap(id)
	} else {
		panic(fmt.Errorf("no such variable %s", id))
	}
}

func (ctx *Frame) compileSetVar(id string) {
	if l := ctx.findLocal(id); l != nil {
		ctx.compileLocalAddr(*l)
		ctx.assembleInst("s0")
		ctx.assembleInst("set")
	} else if c := ctx.findCapture(id); c != nil {
		ctx.compileCaptureAddr(*c)
		ctx.assembleInst("set")
	} else {
		panic(fmt.Errorf("no such variable %s", id))
	}
}

type Lvalue struct {
	Variable   *string
	IndexedExp *IndexedExp
}

func expToLvalue(e *Expression) Lvalue {
	if e.Expression != nil {
		e := e.Expression
		if e.Right == nil {
			e := e.Left
			if e.LogicNotExp == nil {
				e := e.PushExp
				if e.Right == nil {
					e := e.Left
					if e.Right == nil {
						e := e.Left
						if e.Right == nil {
							e := e.Left
							if e.Right == nil {
								e := e.Left
								if e.Right == nil {
									e := e.Left
									if e.Right == nil {
										e := e.Left
										if e.PostfixExp != nil {
											e := e.PostfixExp
											if e.Calls == nil {
												e := e.Function
												if e.Indexes != nil {
													return Lvalue{IndexedExp: e}
												} else {
													e := e.Object
													if e.Variable != nil {
														return Lvalue{Variable: e.Variable}
													}
												}
											}

										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	panic("invalid lvalue")
}

func (ctx *Frame) compileAssignLvalue(e *Expression) {
	lv := expToLvalue(e)
	if lv.IndexedExp == nil {
		ctx.compileSetVar(*lv.Variable)
	} else {
		ctx.compileIndexedExp(lv.IndexedExp.Object, lv.IndexedExp.Indexes, true)
	}
}

func (ctx *Frame) tearDownBlock() {
	if ctx.sp < ctx.baseSp {
		panic(fmt.Sprintf("frame sp %d is below baseSp %d\n", ctx.sp, ctx.baseSp))
	}
	// Set result
	ctx.assembleInt(-int(ctx.sp - ctx.baseSp + 3))
	ctx.assembleInst("s0")
	ctx.assembleInst("set")
	// Drop remaining stack items in this frame, except for return address
	for range ctx.sp - ctx.baseSp {
		ctx.assembleInst("drop")
	}
}

// We do not destroy any information in the frame when returning in case the
// frame lives on to be used for captures.
func (ctx *Frame) assembleReturn() {
	// Get return information
	ctx.assembleInt(int(ctx.nargs))
	ctx.assembleInst("s0")
	ctx.assembleInst("get")
	ctx.assembleInt(int(ctx.nargs + 1))
	ctx.assembleInst("s0")
	ctx.assembleInst("get")
	// Extract return value
	ctx.assembleInt(-3)
	ctx.assembleInst("s0")
	ctx.assembleInst("extract")
	ctx.assembleTrap("ret")
}

var nextLabel uint = 0

func newLabel() string {
	nextLabel += 1
	return fmt.Sprintf("$%d", nextLabel)
}

// FIXME: Separate Block (no args) from Frame
type Frame struct {
	parent   *Frame // lexically enclosing scope
	label    string
	locals   []Local
	captures *[]Capture
	asm      *assembler
	baseSp   libsam.Word
	sp       libsam.Word
	nargs    libsam.Uword // Number of arguments to this frame
	loop     *Frame       // Innermost loop, if any
}

func (ctx *Frame) prepareInst(inst string) string {
	instName := strings.Fields(inst)[0]
	switch instName {
	case "trap":
		panic("use assembleTrap to assemble a trap instruction")
	case "quote":
		panic("use assembleQuote to assemble a quote instruction")
	}
	delta, ok := libsam.StackDifference[instName]
	if !ok {
		panic(fmt.Errorf("invalid instruction %s", instName))
	}
	ctx.adjustSp(delta)
	return instName
}

func (ctx *Frame) assembleInst(inst string) {
	instName := ctx.prepareInst(inst)
	ctx.asm.addInstruction(libsam.Instructions[instName])
}

func (ctx *Frame) assembleSingle(inst string) {
	instName := ctx.prepareInst(inst)
	ctx.asm.addSingleInstruction(libsam.Instructions[instName])
}

func (ctx *Frame) assembleNull() {
	ctx.asm.addNull()
	ctx.adjustSp(1)
}

func (ctx *Frame) assembleInt(n int) {
	ctx.asm.addInt(libsam.Word(n))
	ctx.adjustSp(1)
}

func (ctx *Frame) assembleFloat(f float64) {
	ctx.asm.addFloat(f)
	ctx.adjustSp(1)
}

func (ctx *Frame) assembleBlob(s libsam.Array) {
	ctx.asm.addStack(s)
	ctx.adjustSp(1)
}

func (ctx *Frame) assembleCode(asm *assembler) {
	asm.flushInstructions()
	ctx.assembleBlob(asm.stack)
}

func (ctx *Frame) assembleQuote(inst string) {
	ctx.assembleTrap("quote")
	ctx.assembleSingle(inst)
	// Now undo the stack effect of the instruction we just compiled
	delta := libsam.StackDifference[inst]
	ctx.adjustSp(-delta)
}

func trapStackEffect(function string) libsam.StackEffect {
	stackEffect, ok := libsam.TrapStackEffect[strings.ToUpper(function)]
	if !ok {
		panic(fmt.Errorf("unknown trap %s", function))
	}
	return stackEffect
}

func (ctx *Frame) assembleTrap(trapName string) {
	function, ok := libsam.Traps[strings.ToUpper(trapName)]
	if !ok {
		panic(fmt.Errorf("unknown trap %s", trapName))
	}
	ctx.asm.addTrap(libsam.Uword(function))
	stackEffect := trapStackEffect(trapName)
	ctx.adjustSp(int(stackEffect.Out) - int(stackEffect.In))
}

func (ctx *Frame) assembleBreak() {
	// Drop items down to loop start
	for range ctx.sp - ctx.loop.baseSp {
		ctx.assembleInst("drop")
	}
	ctx.assembleInst("zero")
	ctx.assembleInst("while")
}

func (ctx *Frame) newBlock(loop bool) Frame {
	baseSp := libsam.Word(int(ctx.sp) + 2)
	blockCtx := Frame{
		parent:   ctx.parent,
		label:    newLabel(),
		locals:   slices.Clone(ctx.locals),
		captures: ctx.captures,
		asm:      &assembler{stack: libsam.NewArray()},
		baseSp:   baseSp,
		sp:       baseSp,
		nargs:    ctx.nargs,
		loop:     ctx.loop,
	}
	if loop {
		blockCtx.loop = &blockCtx
	}
	return blockCtx
}

func (ctx *Frame) assembleBlock(b *Block, loop bool) Frame {
	blockCtx := b.Compile(ctx, loop)
	if b.Body.Terminator == nil && blockCtx.asm.stack.Sp() > 0 {
		blockCtx.tearDownBlock()
	}
	return blockCtx
}

func (ctx *Frame) assembleLoop(bodyCtx *Frame) {
	for range bodyCtx.sp - bodyCtx.baseSp {
		bodyCtx.assembleInst("drop")
	}
	bodyCtx.assembleCode(bodyCtx.asm)
	bodyCtx.assembleSingle("go")
	ctx.assembleCode(bodyCtx.asm)
	ctx.assembleInst("do")
}

func Sal(src string, ast bool) libsam.Array {
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
	captures := make([]Capture, 0)
	ctx := Frame{
		locals:   make([]Local, 0),
		captures: &captures,
		asm:      &assembler{stack: libsam.NewArray()},
	}
	ctx.assembleNull() // value of top level
	blockCtx := ctx.assembleBlock(&block, false)
	ctx.assembleCode(blockCtx.asm)
	ctx.assembleInst("do")
	ctx.assembleInst("halt")

	return ctx.asm.stack
}
