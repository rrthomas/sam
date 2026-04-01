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

type PrimaryExp struct {
	Pos lexer.Position

	Null      bool        `  @"null"`
	Bool      *Boolean    `| @("false" | "true")`
	Int       *int64      `| @Int`
	Float     *float64    `| @Float`
	String    *string     `| @String`
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

type Trap struct {
	Pos lexer.Position

	Function  *string       `@Ident`
	Arguments *[]Expression `("," @@)*`
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
	Trap         *Trap          `| @@ ";"`
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
		ctx.assemble("null")
	} else if e.Bool != nil {
		if *e.Bool {
			ctx.assemble("true")
		} else {
			ctx.assemble("false")
		}
	} else if e.Int != nil {
		ctx.assemble(fmt.Sprintf("int %d", *e.Int))
	} else if e.Float != nil {
		ctx.assemble(fmt.Sprintf("float %g", *e.Float))
	} else if e.String != nil {
		ctx.assemble(fmt.Sprintf("string \"%s\"", *e.String)) // FIXME
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
			ctx.assemble("new")
			for _, e := range *e.Container {
				e.Key.Compile(ctx)
				ctx.assemble("int -2", "s0", "get", "append")
			}
		} else {
			ctx.assembleTrap("new_map")
			for _, e := range *e.Container {
				e.Value.Compile(ctx)
				e.Key.Compile(ctx)
				ctx.assemble("int -3", "s0", "get", "set")
			}
		}
	} else if e.EmptyMap {
		ctx.assembleTrap("new_map")
	} else if e.Variable != nil {
		ctx.compileGetVar(*e.Variable)
	} else if e.Block != nil {
		ctx.assemble("null") // value of block
		ctx.assemble(ctx.assembleBlock(e.Block, false).asm)
		ctx.assemble("do")
	} else if e.Function != nil {
		e.Function.Compile(ctx)
	} else if e.Paren != nil {
		e.Paren.Compile(ctx)
	} else { // empty list
		ctx.assemble("new")
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
			ctx.assemble("get")
		}
		if set {
			ctx.assemble("set")
		} else {
			ctx.assemble("get")
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
			ctx.assemble("not")
		case "+":
			break // no-op
		case "-":
			ctx.assemble("neg")
		case "#":
			ctx.assembleTrap("size")
		case "<<<":
			ctx.assemble("shift")
		default:
			panic(fmt.Errorf("unknown UnaryExp.PreOp %s", e.PreOp))
		}
	} else if e.PostfixExp != nil {
		e.PostfixExp.Compile(ctx)

		// Deal with PostOp, if any
		if e.PostOp != nil {
			switch *e.PostOp {
			case ">>>":
				ctx.assemble("pop")
			default:
				panic(fmt.Errorf("unknown UnaryExp.PostOp %s", *e.PostOp))
			}
		}
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
			ctx.assemble("zero", "new", "call")
			if args.Arguments != nil {
				ctx.adjustSp(-(len(*args.Arguments)))
			}
		}
	}
}

func (a *Args) Compile(ctx *Frame) {
	// Push arguments
	if a.Arguments == nil {
		ctx.assemble("zero")
		return
	}
	for _, a := range *a.Arguments {
		a.Compile(ctx)
	}
	// Push number of arguments
	ctx.assemble(fmt.Sprintf("int %d", len(*a.Arguments)))
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
			ctx.assemble("_two", "s0", "extract", "lt", "not", "neg")
		case ">":
			ctx.assemble("_two", "s0", "extract", "lt", "neg")
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
			ctx.assemble("_two", "s0", "get", "append")
		case ">>": // List prepend: i >> l
			e.Right.Compile(ctx)
			e.Left.Compile(ctx)
			ctx.assemble("_two", "s0", "get", "prepend")
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
		ctx.assemble("neg", "not", "neg")
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
			ctx.assemble("null")
			thenCtx := ctx.newBlock(false)
			e.Right.Compile(&thenCtx)
			thenCtx.tearDownBlock()
			elseCtx := ctx.newBlock(false)
			elseCtx.assemble("false")
			elseCtx.tearDownBlock()
			e.Left.Compile(ctx)
			ctx.assemble(thenCtx.asm)
			ctx.assemble(elseCtx.asm)
			ctx.assemble("if")
		case "or":
			ctx.assemble("null")
			elseCtx := ctx.newBlock(false)
			e.Right.Compile(&elseCtx)
			elseCtx.tearDownBlock()
			thenCtx := ctx.newBlock(false)
			thenCtx.assemble("true")
			thenCtx.tearDownBlock()
			e.Left.Compile(ctx)
			ctx.assemble(thenCtx.asm)
			ctx.assemble(elseCtx.asm)
			ctx.assemble("if")
		default:
			panic(fmt.Errorf("unknown LogicExp.Op %s", e.Op))
		}
	}
}

func (e *Expression) Compile(ctx *Frame) {
	if e.Ifs != nil {
		e.Ifs.Compile(ctx)
	} else if e.Loop != nil {
		ctx.assemble("null")
		blockCtx := e.Loop.Compile(ctx, true)
		ctx.assembleLoop(&blockCtx)
	} else if e.ForVar != "" {
		// Initialize iterator
		ctx.locals = append(ctx.locals, Local{id: "$iter", pos: int(ctx.sp)})
		e.Iter.Compile(ctx)
		ctx.assembleTrap("iter")
		// Loop body
		ctx.assemble("null")
		blockCtx := ctx.newBlock(true)
		// Call iterator and test for termination
		blockCtx.locals = append(blockCtx.locals, Local{id: e.ForVar, pos: int(blockCtx.sp)})
		blockCtx.compileGetVar("$iter")
		blockCtx.assembleTrap("next")
		blockCtx.assemble("null") // return value
		thenCtx := blockCtx.newBlock(false)
		thenCtx.assembleBreak()
		elseCtx := blockCtx.assembleBlock(e.Body, false)
		blockCtx.assemble("_two", "s0", "get", "null", "eq")
		blockCtx.assemble(thenCtx.asm)
		blockCtx.assemble(elseCtx.asm)
		blockCtx.assemble("if")
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
	ctx.assemble("null") // return value
	thenCtx := ctx.assembleBlock((*il)[0].Then, false)
	elseCtx := Frame{}
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
	ctx.assemble(thenCtx.asm)
	ctx.assemble(elseCtx.asm)
	ctx.assemble("if")
}

func (i *Ifs) Compile(ctx *Frame) {
	ctx.compileIfs(i.IfList, i.FinalElse)
}

// Assignment, or an rvalue that is syntactically an lvalue
func (a *Assignment) Compile(ctx *Frame) {
	if a.Expression != nil {
		a.Expression.Compile(ctx)
		ctx.assemble("_one", "s0", "get")
		ctx.compileAssignLvalue(a.Lvalue)
	} else {
		a.Lvalue.Compile(ctx)
	}
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
		ctx.assemble("null")
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
	} else if s.Trap != nil {
		s.Trap.Compile(ctx)
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
			ctx.assemble("null")
		}
		ctx.assemble(fmt.Sprintf("int %d", ctx.loop.baseSp-(ctx.sp+3)), "s0", "set")
		ctx.assembleBreak()
	} else if t.Continue {
		if ctx.loop == nil {
			panic("'continue' used outside a loop")
		}
		// Drop items down to loop start
		for range ctx.sp - ctx.loop.baseSp {
			ctx.assemble("drop")
		}
		ctx.assemble(fmt.Sprintf("stack %s", ctx.loop.label))
		ctx.assemble("go")
	} else {
		panic("invalid Terminator")
	}
}

func (b *Body) Compile(ctx *Frame) {
	if b.Statements != nil {
		for i, s := range *b.Statements {
			s.Compile(ctx)
			if s.Declarations == nil && (i < len(*b.Statements)-1 || b.Terminator != nil) {
				ctx.assemble("drop")
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
		asm:      make([]any, 0),
		baseSp:   0,
		sp:       libsam.Word(nargs) + 2, // exclude PC & P0 from count
		nargs:    nargs,
	}
	if f.Parameters != nil {
		for i, p := range *f.Parameters {
			innerCtx.locals = append(innerCtx.locals, Local{id: p, pos: i})
		}
	}
	ctx.assemble("new") // closure array
	ctx.assemble("new") // captures array
	blockCtx := f.Body.Compile(&innerCtx, false)
	ctx.assemble("_two", "s0", "get", "append") // append captures to closure
	if f.Body.Body.Terminator == nil {
		blockCtx.assembleReturn()
	}
	ctx.assemble(blockCtx.asm)
	ctx.assemble("_two", "s0", "get", "append") // append code to closure
	ctx.assembleQuote("go")
	ctx.assemble("_two", "s0", "get", "append") // append tail call to closure
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
			parent.assemble("s0", "_two", "s0", "get", "append")
			parent.assemble(fmt.Sprintf("int %d", int(l.pos)))
			parent.assemble("_two", "s0", "get", "append")
			found = true
		} else if i := parent.findCapture(id); i != nil {
			// Append address of capture to captures array
			parent.compileCaptureAddr(*i)
			parent.assemble("int -3", "s0", "get", "append")
			parent.assemble("_two", "s0", "get", "append")
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
	ctx.assemble(fmt.Sprintf("int %d", int(l.pos)))
}

func (ctx *Frame) compileCaptureAddr(i uint) {
	ctx.assemble(
		fmt.Sprintf("int %d", ctx.nargs+3), "s0", "get",
		fmt.Sprintf("int %d", i*2+1),
		"_two", "s0", "get",
		"get",
		fmt.Sprintf("int %d", i*2),
		"int -3", "s0", "extract",
		"get",
	)
}

func (ctx *Frame) compileGetVar(id string) {
	if l := ctx.findLocal(id); l != nil {
		ctx.compileLocalAddr(*l)
		ctx.assemble("s0", "get")
	} else if c := ctx.findCapture(id); c != nil {
		ctx.compileCaptureAddr(*c)
		ctx.assemble("get")
	} else if _, ok := libsam.TrapStackEffect[strings.ToUpper(id)]; ok {
		ctx.assembleTrap(id)
	} else {
		panic(fmt.Errorf("no such variable %s", id))
	}
}

func (ctx *Frame) compileSetVar(id string) {
	if l := ctx.findLocal(id); l != nil {
		ctx.compileLocalAddr(*l)
		ctx.assemble("s0", "set")
	} else if c := ctx.findCapture(id); c != nil {
		ctx.compileCaptureAddr(*c)
		ctx.assemble("set")
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
	ctx.assemble(fmt.Sprintf("int %d", -int(ctx.sp-ctx.baseSp+3)), "s0", "set")
	// Drop remaining stack items in this frame, except for return address
	for range ctx.sp - ctx.baseSp {
		ctx.assemble("drop")
	}
}

// We do not destroy any information in the frame when returning in case the
// frame lives on to be used for captures.
func (ctx *Frame) assembleReturn() {
	// Get return information
	ctx.assemble(fmt.Sprintf("int %d", ctx.nargs), "s0", "get")
	ctx.assemble(fmt.Sprintf("int %d", ctx.nargs+1), "s0", "get")
	ctx.assemble(fmt.Sprintf("int %d", ctx.nargs+2), "s0", "get")
	// Extract return value
	ctx.assemble("int -4", "s0", "extract")
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
	asm      []any
	baseSp   libsam.Word
	sp       libsam.Word
	nargs    libsam.Uword // Number of arguments to this frame
	loop     *Frame       // Innermost loop, if any
}

func (ctx *Frame) assemble(insts ...any) {
	for _, i := range insts {
		switch inst := i.(type) {
		case string:
			ctx.assembleInst(inst)
		default: // Assume a block (array or map)
			ctx.adjustSp(1) // A stack pushes a `ref` instruction
			ctx.asm = append(ctx.asm, i)
		}
	}
}

func (ctx *Frame) assembleInst(inst string) {
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
	if instName == "null" {
		ctx.asm = append(ctx.asm, nil)
	} else {
		ctx.asm = append(ctx.asm, inst)
	}
}

func (ctx *Frame) assembleSingle(inst string) {
	// Assemble instruction
	ctx.assembleInst(inst)
	// Overwrite instruction
	ctx.asm[len(ctx.asm)-1] = map[string]string{"!single": inst}
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

func (ctx *Frame) assembleTrap(function string) {
	ctx.asm = append(ctx.asm, fmt.Sprintf("trap %s", function))
	stackEffect := trapStackEffect(function)
	ctx.adjustSp(int(stackEffect.Out) - int(stackEffect.In))
}

func (ctx *Frame) assembleBreak() {
	// Drop items down to loop start
	for range ctx.sp - ctx.loop.baseSp {
		ctx.assemble("drop")
	}
	ctx.assemble("zero", "while")
}

func (ctx *Frame) newBlock(loop bool) Frame {
	baseSp := libsam.Word(int(ctx.sp) + 2)
	blockCtx := Frame{
		parent:   ctx.parent,
		label:    newLabel(),
		locals:   slices.Clone(ctx.locals),
		captures: ctx.captures,
		asm:      make([]any, 0),
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
	if b.Body.Terminator == nil {
		blockCtx.tearDownBlock()
	}
	return blockCtx
}

func (ctx *Frame) assembleLoop(bodyCtx *Frame) {
	for range bodyCtx.sp - bodyCtx.baseSp {
		bodyCtx.assemble("drop")
	}
	bodyCtx.assemble(fmt.Sprintf("stack %s", bodyCtx.label))
	bodyCtx.assembleSingle("go")
	// Add loop label to start of block
	if len(bodyCtx.asm) > 0 {
		bodyCtx.asm[0] = map[string]any{bodyCtx.label: bodyCtx.asm[0]}
	}
	ctx.assemble(bodyCtx.asm)
	ctx.assemble("do")
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
	captures := make([]Capture, 0)
	ctx := Frame{locals: make([]Local, 0), captures: &captures, asm: make([]any, 0)}
	ctx.assemble("null") // value of top level
	blockCtx := ctx.assembleBlock(&block, false)
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
