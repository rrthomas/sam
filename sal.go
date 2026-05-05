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

	Ifs        *Ifs            `  @@`
	Loop       *Block          `| "loop" @@`
	Asm        *[]AsmStatement `| "asm" "{" (@@)* "}"`
	ForVar     string          `| "for" @Ident "in"`
	Iter       *Expression     `  @@`
	Body       *Block          `  @@`
	Expression *LogicExp       `| @@`
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

type Use struct {
	Pos lexer.Position

	Path *[]string `"use" @Ident ("." @Ident)*`
}

type Assignment struct {
	Pos lexer.Position

	Lvalue     *Expression `@@ [":="`
	Expression *Expression `@@]`
}

type Statement struct {
	Pos lexer.Position

	Empty        bool           `  @";"`
	Use          *Use           `| @@ ";"`
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

type AsmStatement struct {
	Pos lexer.Position

	Trap       *string     `  "trap" @Ident ";"`
	Single     *string     `| "single" @Ident ";"`
	Quote      *string     `| "quote" @Ident ";"`
	QuoteTrap  *string     `| "quote" "trap" @Ident ";"`
	Expression *PrimaryExp `| @@ ";"`
}

type Function struct {
	Pos lexer.Position

	Parameters *[]string `"fn" "(" [@Ident ("," @Ident)* ","?] ")"`
	Body       *Block    `@@`
}

// Compilation

func (ctx *Scope) compileBlock(b *Block) {
	ctx.compileNull() // value of block
	blockCtx := ctx.newBlock(false)
	b.Body.Compile(&blockCtx) // body of block
	if b.Body.Terminator == nil {
		if blockCtx.frame.sp < blockCtx.baseSp {
			panic(fmt.Sprintf("frame sp %d is below baseSp %d\n", blockCtx.frame.sp, blockCtx.baseSp))
		}
		// Set result
		blockCtx.compileInt(-int(blockCtx.frame.sp - blockCtx.baseSp + 1))
		blockCtx.compileInst("s0")
		blockCtx.compileInst("set")
		// Drop remaining stack items in this block
		for range blockCtx.frame.sp - blockCtx.baseSp {
			blockCtx.compileInst("drop")
		}
	}
}

func (e *PrimaryExp) Compile(ctx *Scope) {
	if e.Null {
		ctx.compileNull()
	} else if e.Bool != nil {
		if *e.Bool {
			ctx.compileBool(true)
		} else {
			ctx.compileBool(false)
		}
	} else if e.Int != nil {
		ctx.compileInt(int(*e.Int))
	} else if e.Float != nil {
		ctx.compileFloat(*e.Float)
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
		ctx.compileBlob(libsam.NewString(str))
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
			ctx.compileInst("new")
			for _, e := range *e.Container {
				e.Key.Compile(ctx)
				ctx.compileInst("_two")
				ctx.compileInst("s0")
				ctx.compileInst("get")
				ctx.compileInst("append")
			}
		} else {
			ctx.compileTrap("new_map")
			for _, e := range *e.Container {
				e.Value.Compile(ctx)
				e.Key.Compile(ctx)
				ctx.compileInt(-3)
				ctx.compileInst("s0")
				ctx.compileInst("get")
				ctx.compileInst("set")
			}
		}
	} else if e.EmptyMap {
		ctx.compileTrap("new_map")
	} else if e.Variable != nil {
		ctx.compileGetVar(*e.Variable)
	} else if e.Block != nil {
		ctx.compileBlock(e.Block)
	} else if e.Function != nil {
		e.Function.Compile(ctx)
	} else if e.Paren != nil {
		e.Paren.Compile(ctx)
	} else { // empty list
		ctx.compileInst("new")
	}
}

func (ctx *Scope) compileIndexedExp(object *PrimaryExp, indexes *[]Expression, set bool) {
	if indexes != nil {
		n_indexes := len(*indexes)
		for i := range *indexes {
			(*indexes)[n_indexes-i-1].Compile(ctx)
		}
	}
	object.Compile(ctx)
	if indexes != nil {
		for range len(*indexes) - 1 {
			ctx.compileInst("get")
		}
		if set {
			ctx.compileInst("set")
		} else {
			ctx.compileInst("get")
		}
	}

}

func (e *IndexedExp) Compile(ctx *Scope) {
	ctx.compileIndexedExp(e.Object, e.Indexes, false)
}

func (e *UnaryExp) Compile(ctx *Scope) {
	if e.PrefixUnaryExp != nil {
		e.PrefixUnaryExp.Compile(ctx)
		switch e.PreOp {
		case "~":
			ctx.compileInst("not")
		case "+":
			break // no-op
		case "-":
			ctx.compileInst("neg")
		case "#":
			ctx.compileTrap("size")
		case "<<<":
			ctx.compileInst("shift")
		default:
			panic(fmt.Errorf("unknown UnaryExp.PreOp %s", e.PreOp))
		}
	} else if e.PostfixExp != nil {
		e.PostfixExp.Compile(ctx)

		// Deal with PostOp, if any
		if e.PostOp != nil {
			switch *e.PostOp {
			case ">>>":
				ctx.compileInst("pop")
			default:
				panic(fmt.Errorf("unknown UnaryExp.PostOp %s", *e.PostOp))
			}
		}
	} else {
		panic("invalid UnaryExp")
	}
}

func (e *CallExp) Compile(ctx *Scope) {
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
				ctx.compileInst("new")
				ctx.compileInst("call")
				if args.Arguments != nil {
					ctx.adjustSp(-(len(*args.Arguments)))
				}
			}
		}
	}
}

func (a *Args) Compile(ctx *Scope) {
	// Push arguments
	if a.Arguments == nil {
		ctx.compileInst("zero")
		return
	}
	for _, a := range *a.Arguments {
		a.Compile(ctx)
	}
	// Push number of arguments
	ctx.compileInt(len(*a.Arguments))
}

func (e *ExponentExp) Compile(ctx *Scope) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		ctx.compileTrap("pow")
	}
}

func (e *ProductExp) Compile(ctx *Scope) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "*":
			ctx.compileInst("mul")
		case "/":
			ctx.compileTrap("div")
		case "%":
			ctx.compileTrap("rem")
		default:
			panic(fmt.Errorf("unknown ProductExp.Op %s", e.Op))
		}
	}
}

func (e *SumExp) Compile(ctx *Scope) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "+":
			ctx.compileInst("add")
		case "-":
			ctx.compileInst("neg")
			ctx.compileInst("add")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *CompareExp) Compile(ctx *Scope) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "==":
			ctx.compileInst("eq")
		case "!=":
			ctx.compileInst("eq")
			ctx.compileInst("not")
		case "<":
			ctx.compileInst("lt")
		case "<=":
			ctx.compileInst("_two")
			ctx.compileInst("s0")
			ctx.compileInst("extract")
			ctx.compileInst("lt")
			ctx.compileInst("not")
		case ">":
			ctx.compileInst("_two")
			ctx.compileInst("s0")
			ctx.compileInst("extract")
			ctx.compileInst("lt")
		case ">=":
			ctx.compileInst("lt")
			ctx.compileInst("not")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *BitwiseExp) Compile(ctx *Scope) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "&":
			ctx.compileInst("and")
		case "^":
			ctx.compileInst("xor")
		case "|":
			ctx.compileInst("or")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *PushExp) Compile(ctx *Scope) {
	if e.Right != nil {
		switch e.Op {
		case "<<": // List append: l << i
			e.Left.Compile(ctx)
			e.Right.Compile(ctx)
			ctx.compileInst("_two")
			ctx.compileInst("s0")
			ctx.compileInst("get")
			ctx.compileInst("append")
		case ">>": // List prepend: i >> l
			e.Right.Compile(ctx)
			e.Left.Compile(ctx)
			ctx.compileInst("_two")
			ctx.compileInst("s0")
			ctx.compileInst("get")
			ctx.compileInst("prepend")
		default:
			panic(fmt.Errorf("unknown PushExp.Op %s", e.Op))
		}
	} else {
		e.Left.Compile(ctx)
	}
}

func (e *LogicNotExp) Compile(ctx *Scope) {
	if e.LogicNotExp != nil {
		e.LogicNotExp.Compile(ctx)
		ctx.compileInst("not")
	} else if e.PushExp != nil {
		e.PushExp.Compile(ctx)
	} else {
		panic("invalid LogicNotExp")
	}

}

func (e *LogicExp) Compile(ctx *Scope) {
	if e.Right == nil {
		e.Left.Compile(ctx)
	} else {
		switch e.Op {
		case "and":
			ctx.compileIf(
				func(ctx *Scope) { e.Left.Compile(ctx) },
				func(ctx *Scope) { e.Right.Compile(ctx) },
				func(ctx *Scope) { ctx.compileBool(false) },
			)
		case "or":
			ctx.compileIf(
				func(ctx *Scope) { e.Left.Compile(ctx) },
				func(ctx *Scope) { ctx.compileBool(true) },
				func(ctx *Scope) { e.Right.Compile(ctx) },
			)
		default:
			panic(fmt.Errorf("unknown LogicExp.Op %s", e.Op))
		}
	}
}

func (e *Expression) Compile(ctx *Scope) {
	if e.Ifs != nil {
		e.Ifs.Compile(ctx)
	} else if e.Loop != nil {
		ctx.compileNull()
		blockCtx := e.Loop.Compile(ctx, true)
		ctx.compileLoop(&blockCtx)
	} else if e.Asm != nil {
		for _, s := range *e.Asm {
			s.Compile(ctx)
		}
	} else if e.ForVar != "" {
		// Initialize iterator
		ctx.locals = append(ctx.locals, Local{id: "$iter", pos: int(ctx.frame.sp)})
		e.Iter.Compile(ctx)
		ctx.compileTrap("iter")
		// Loop body
		ctx.compileNull()
		blockCtx := ctx.newBlock(true)
		// Call iterator and test for termination
		blockCtx.locals = append(blockCtx.locals, Local{id: e.ForVar, pos: int(blockCtx.frame.sp)})
		blockCtx.compileGetVar("$iter")
		blockCtx.compileTrap("next")
		blockCtx.compileNull() // return value
		blockCtx.compileIf(
			func(ctx *Scope) {
				ctx.compileInst("_two")
				ctx.compileInst("s0")
				ctx.compileInst("get")
				ctx.compileNull()
				ctx.compileInst("eq")
			},
			func(ctx *Scope) { ctx.compileBreak() },
			func(ctx *Scope) { ctx.compileBlock(e.Body) },
		)
		// Complete the loop body and add to the current context
		ctx.compileLoop(&blockCtx)
	} else if e.Expression != nil {
		e.Expression.Compile(ctx)
	} else {
		panic("invalid Expression")
	}
}

type compiler = func(ctx *Scope)

func (ctx *Scope) compileIf(cond compiler, then compiler, else_ compiler) {
	if cond == nil || then == nil {
		panic("compileIf: cond and then cannot be nil")
	}
	cond(ctx)
	ctx.compileInt(0) // space for jump target
	ifJumpAddr := libsam.Uword(ctx.frame.asm.array.Sp() - 1)
	ctx.compileTrap("jump_if_false")
	ifJumpSp := ctx.frame.sp
	then(ctx)
	var thenJumpAddr libsam.Uword
	ctx.compileInt(0) // space for jump target
	thenJumpAddr = libsam.Uword(ctx.frame.asm.array.Sp() - 1)
	ctx.compileTrap("jump")
	ctx.frame.asm.flushInstructions()
	ctx.frame.asm.array.Poke(ifJumpAddr, libsam.Uword(libsam.MakeInstInt(libsam.Word(ctx.frame.asm.array.Sp()-ifJumpAddr-2))))
	ctx.frame.sp = ifJumpSp
	if else_ != nil {
		else_(ctx)
	} else {
		ctx.compileNull()
	}
	ctx.frame.asm.flushInstructions()
	ctx.frame.asm.array.Poke(thenJumpAddr, libsam.Uword(libsam.MakeInstInt(libsam.Word(ctx.frame.asm.array.Sp()-thenJumpAddr-2))))
}

func (ctx *Scope) compileIfs(il *[]If, fe *Block) {
	if il == nil || len(*il) == 0 {
		panic("unexpected nil or empty IfList")
	}
	ctx.compileIf(
		func(ctx *Scope) { (*il)[0].Cond.Compile(ctx) },
		func(ctx *Scope) { ctx.compileBlock((*il)[0].Then) },
		func(ctx *Scope) {
			if len(*il) == 1 {
				if fe != nil {
					ctx.compileBlock(fe)
				} else {
					ctx.compileNull()
				}
			} else {
				restIl := (*il)[1:]
				restIfs := Ifs{Pos: (*il)[0].Pos, IfList: &restIl, FinalElse: fe}
				restIfs.Compile(ctx)
			}
		},
	)
}

func (i *Ifs) Compile(ctx *Scope) {
	ctx.compileIfs(i.IfList, i.FinalElse)
}

func (a *AsmStatement) Compile(ctx *Scope) {
	if a.Trap != nil {
		ctx.compileTrap(*a.Trap)
	} else if a.Single != nil {
		ctx.compileSingle(*a.Single)
	} else if a.Quote != nil {
		ctx.compileQuote(*a.Quote)
	} else if a.QuoteTrap != nil {
		ctx.compileQuoteTrap(*a.QuoteTrap)
	} else if a.Expression != nil {
		if a.Expression.Variable != nil {
			if insn, ok := libsam.Instructions[strings.ToLower(*a.Expression.Variable)]; ok {
				ctx.frame.asm.addInstruction(insn)
			} else {
				a.Expression.Compile(ctx)
			}
		} else {
			a.Expression.Compile(ctx)
		}
	} else {
		panic(fmt.Errorf("invalid AsmStatement"))
	}
}

// Assignment, or an rvalue that is syntactically an lvalue
func (a *Assignment) Compile(ctx *Scope) {
	if a.Expression != nil {
		a.Expression.Compile(ctx)
		ctx.compileInst("_one")
		ctx.compileInst("s0")
		ctx.compileInst("get")
		ctx.compileAssignLvalue(a.Lvalue)
	} else {
		a.Lvalue.Compile(ctx)
	}
}

func (ctx *Scope) compileTrapCall(function string, args *[]Expression) {
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
		ctx.compileNull()
	}
}

func (d *Declaration) Compile(ctx *Scope) {
	ctx.locals = append(ctx.locals, Local{id: *d.Variable, pos: int(ctx.frame.sp)})
	d.Value.Compile(ctx)
}

func (s *Statement) Compile(ctx *Scope) {
	if s.Empty {
		// Do nothing
	} else if s.Assignment != nil {
		s.Assignment.Compile(ctx)
	} else if s.Declarations != nil {
		for _, d := range *s.Declarations {
			d.Compile(ctx)
		}
	} else if s.Use != nil {
		filename := strings.Join(*s.Use.Path, ".")
		if src, err := os.ReadFile(filename); err == nil {
			body := parseSource(string(src))
			body.Compile(ctx)
		}
	} else {
		panic("invalid Statement")
	}
}

func (t *Terminator) Compile(ctx *Scope) {
	if t.Return != nil {
		t.Return.Compile(ctx)
		ctx.compileTrap("ret")
	} else if t.BreakExp != nil || t.Break {
		if ctx.loop == nil {
			panic("'break' used outside a loop")
		}
		// Set loop value
		if t.BreakExp != nil {
			t.BreakExp.Compile(ctx)
		} else {
			ctx.compileNull()
		}
		ctx.compileInt(int(ctx.loop.baseSp - (ctx.frame.sp + 1)))
		ctx.compileInst("s0")
		ctx.compileInst("set")
		ctx.compileBreak()
	} else if t.Continue {
		if ctx.loop == nil {
			panic("'continue' used outside a loop")
		}
		// Drop items down to loop start
		if ctx.frame.sp < ctx.loop.baseSp {
			panic(fmt.Sprintf("frame sp %d is below loop's baseSp %d\n", ctx.frame.sp, ctx.loop.baseSp))
		}
		for range ctx.frame.sp - ctx.loop.baseSp {
			ctx.compileInst("drop")
		}
		ctx.compileInt(int(ctx.loop.initialPc - ctx.frame.asm.array.Sp() - 3))
		ctx.compileTrap("jump")
	} else {
		panic("invalid Terminator")
	}
}

func (b *Body) Compile(ctx *Scope) {
	if b.Statements != nil {
		for i, s := range *b.Statements {
			s.Compile(ctx)
			if s.Assignment != nil && (i < len(*b.Statements)-1 || b.Terminator != nil) {
				ctx.compileInst("drop")
			}
		}
	}
	if b.Terminator != nil {
		b.Terminator.Compile(ctx)
	}
}

func (b *Block) Compile(ctx *Scope, loop bool) Scope {
	blockCtx := ctx.newBlock(loop)
	b.Body.Compile(&blockCtx)
	return blockCtx
}

func (f *Function) Compile(ctx *Scope) {
	nargs := libsam.Uword(0)
	if f.Parameters != nil {
		nargs = libsam.Uword(len(*f.Parameters))
	}
	captures := make([]Capture, 0)
	frame := Frame{
		asm: &assembler{array: libsam.NewArray()},
		sp:  libsam.Word(nargs) + 3,
	}
	innerCtx := Scope{
		frame:     &frame,
		parent:    ctx,
		locals:    make([]Local, 0),
		captures:  &captures,
		baseSp:    0,
		exitJumps: make([]libsam.Uword, 0),
	}
	if f.Parameters != nil {
		for i, p := range *f.Parameters {
			innerCtx.locals = append(innerCtx.locals, Local{id: p, pos: i + 3})
		}
	}

	// Compile function body
	blockCtx := f.Body.Compile(&innerCtx, false)
	if f.Body.Body.Terminator == nil {
		blockCtx.compileTrap("ret")
	}

	// Construct closure
	ctx.compileCaptures(&blockCtx)
	ctx.compileCode(blockCtx.frame.asm)
	ctx.compileTrap("new_closure")
	ctx.adjustSp(-(len(*blockCtx.captures) * 2))
}

type Local struct {
	id  string
	pos int // relative to base of stack frame
}

const (
	CaptureNone = iota
	CaptureLocal
	CaptureParent
)

type Capture struct {
	id  string
	ty  int
	pos uint
}

func (ctx *Scope) adjustSp(delta int) {
	// FIXME: check this doesn't go outside some limit.
	ctx.frame.sp += libsam.Word(delta)
}

func (ctx *Scope) findLocal(id string) *Local {
	for i := len(ctx.locals) - 1; i >= 0; i-- {
		l := ctx.locals[i]
		if l.id == id {
			return &l
		}
	}
	return nil
}

func (ctx *Scope) findCapture(id string) *uint {
	for i := len(*ctx.captures) - 1; i >= 0; i-- {
		c := (*ctx.captures)[i]
		if c.id == id {
			index := uint(i)
			return &index
		}
	}
	if parent := ctx.parent; parent != nil {
		var capture *Capture = nil
		if l := parent.findLocal(id); l != nil {
			capture = &Capture{id: id, ty: CaptureLocal, pos: uint(l.pos)}
		} else if i := parent.findCapture(id); i != nil {
			capture = &Capture{id: id, ty: CaptureParent, pos: *i}
		}
		if capture != nil {
			newCaptureIndex := uint(len(*ctx.captures))
			*ctx.captures = append(*ctx.captures, *capture)
			return &newCaptureIndex
		}
	}
	return nil
}

func (ctx *Scope) compileCaptures(blockCtx *Scope) {
	for _, c := range *blockCtx.captures {
		switch c.ty {
		case CaptureLocal:
			ctx.compileInst("s0")
			ctx.compileInt(int(c.pos))
		case CaptureParent:
			ctx.compileCaptureAddr(c.pos)
		default:
			panic(fmt.Errorf("invalid capture %+v", c))
		}
	}
	ctx.compileInt(len(*blockCtx.captures) * 2)
}

func (ctx *Scope) compileCaptureAddr(i uint) {
	ctx.compileInst("two")
	ctx.compileInst("s0")
	ctx.compileInst("get")
	ctx.compileInt(int(i*2 + 1))
	ctx.compileInst("_two")
	ctx.compileInst("s0")
	ctx.compileInst("get")
	ctx.compileInst("get")
	ctx.compileInt(int(i * 2))
	ctx.compileInt(-3)
	ctx.compileInst("s0")
	ctx.compileInst("extract")
	ctx.compileInst("get")
}

func (ctx *Scope) isTrap(id string) bool {
	if ctx.findLocal(id) == nil || ctx.findCapture(id) == nil {
		if _, ok := libsam.TrapStackEffect[strings.ToUpper(id)]; ok {
			return true
		}
	}
	return false
}

func (ctx *Scope) compileGetVar(id string) {
	if l := ctx.findLocal(id); l != nil {
		ctx.compileInt(l.pos)
		ctx.compileInst("s0")
		ctx.compileInst("get")
	} else if c := ctx.findCapture(id); c != nil {
		ctx.compileCaptureAddr(*c)
		ctx.compileInst("get")
	} else if _, ok := libsam.TrapStackEffect[strings.ToUpper(id)]; ok {
		ctx.compileTrap(id)
	} else {
		panic(fmt.Errorf("no such variable %s", id))
	}
}

func (ctx *Scope) compileSetVar(id string) {
	if l := ctx.findLocal(id); l != nil {
		ctx.compileInt(l.pos)
		ctx.compileInst("s0")
		ctx.compileInst("set")
	} else if c := ctx.findCapture(id); c != nil {
		ctx.compileCaptureAddr(*c)
		ctx.compileInst("set")
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

func (ctx *Scope) compileAssignLvalue(e *Expression) {
	lv := expToLvalue(e)
	if lv.IndexedExp == nil {
		ctx.compileSetVar(*lv.Variable)
	} else {
		ctx.compileIndexedExp(lv.IndexedExp.Object, lv.IndexedExp.Indexes, true)
	}
}

type Frame struct {
	asm *assembler
	sp  libsam.Word
}

type Scope struct {
	frame     *Frame
	parent    *Scope // lexically enclosing scope
	locals    []Local
	captures  *[]Capture
	baseSp    libsam.Word
	initialPc libsam.Uword
	loop      *Scope // Innermost loop, if any
	exitJumps []libsam.Uword
}

func (ctx *Scope) prepareInst(inst string) string {
	instName := strings.Fields(inst)[0]
	switch instName {
	case "trap":
		panic("use compileTrap for a trap instruction")
	case "quote":
		panic("use compileQuote for a quote instruction")
	}
	delta, ok := libsam.StackDifference[instName]
	if !ok {
		panic(fmt.Errorf("invalid instruction %s", instName))
	}
	ctx.adjustSp(delta)
	return instName
}

func (ctx *Scope) compileInst(inst string) {
	instName := ctx.prepareInst(inst)
	ctx.frame.asm.addInstruction(libsam.Instructions[instName])
}

func (ctx *Scope) compileSingle(inst string) {
	instName := ctx.prepareInst(inst)
	ctx.frame.asm.addSingleInstruction(libsam.Instructions[instName])
}

func (ctx *Scope) compileNull() {
	ctx.frame.asm.addNull()
	ctx.adjustSp(1)
}

func (ctx *Scope) compileBool(f bool) {
	ctx.frame.asm.addBool(f)
	ctx.adjustSp(1)
}

func (ctx *Scope) compileInt(n int) {
	ctx.frame.asm.addInt(libsam.Word(n))
	ctx.adjustSp(1)
}

func (ctx *Scope) compileFloat(f float64) {
	ctx.frame.asm.addFloat(f)
	ctx.adjustSp(1)
}

func (ctx *Scope) compileBlob(s libsam.Blob) {
	ctx.frame.asm.addBlob(s)
	ctx.adjustSp(1)
}

func (ctx *Scope) compileCode(asm *assembler) {
	asm.flushInstructions()
	ctx.compileBlob(asm.array)
}

func (ctx *Scope) compileQuote(inst string) {
	ctx.compileTrap("quote")
	ctx.compileSingle(inst)
	// Now undo the stack effect of the instruction we just compiled
	delta := libsam.StackDifference[inst]
	ctx.adjustSp(-delta)
}

func (ctx *Scope) compileQuoteTrap(trapName string) {
	ctx.compileTrap("quote")
	ctx.compileTrap(trapName)
	// Now undo the stack effect of the instruction we just compiled
	stackEffect := trapStackEffect(trapName)
	ctx.adjustSp(int(stackEffect.In) - int(stackEffect.Out))
}

func trapStackEffect(function string) libsam.StackEffect {
	stackEffect, ok := libsam.TrapStackEffect[strings.ToUpper(function)]
	if !ok {
		panic(fmt.Errorf("unknown trap %s", function))
	}
	return stackEffect
}

func (ctx *Scope) compileTrap(trapName string) {
	function, ok := libsam.Traps[strings.ToUpper(trapName)]
	if !ok {
		panic(fmt.Errorf("unknown trap %s", trapName))
	}
	ctx.frame.asm.addTrap(libsam.Uword(function))
	stackEffect := trapStackEffect(trapName)
	ctx.adjustSp(int(stackEffect.Out) - int(stackEffect.In))
}

func (ctx *Scope) resolveExitJumps() {
	jumpAddr := ctx.frame.asm.array.Sp()
	for _, addr := range ctx.exitJumps {
		jumpInst := libsam.Uword(libsam.MakeInstInt(libsam.Word(jumpAddr - addr - 2)))
		ctx.frame.asm.array.Poke(addr, jumpInst)
	}
}

func (ctx *Scope) compileBreak() {
	// Drop items down to loop start
	for range ctx.frame.sp - ctx.loop.baseSp {
		ctx.compileInst("drop")
	}
	ctx.compileInt(0) // space for jump target
	ctx.loop.exitJumps = append(ctx.loop.exitJumps, ctx.frame.asm.array.Sp()-1)
	ctx.compileTrap("jump")
}

func (ctx *Scope) newBlock(isLoop bool) Scope {
	blockCtx := Scope{
		frame:     ctx.frame,
		parent:    ctx.parent,
		locals:    slices.Clone(ctx.locals), // FIXME: probe outer blocks instead
		captures:  ctx.captures,
		baseSp:    ctx.frame.sp,
		loop:      ctx.loop,
		initialPc: ctx.frame.asm.array.Sp(),
		exitJumps: make([]libsam.Uword, 0),
	}
	if isLoop {
		blockCtx.loop = &blockCtx
	}
	return blockCtx
}

func (ctx *Scope) compileLoop(bodyCtx *Scope) {
	for range bodyCtx.frame.sp - bodyCtx.baseSp {
		ctx.compileInst("drop")
	}
	ctx.compileInt(int(bodyCtx.initialPc - ctx.frame.asm.array.Sp() - 3))
	ctx.compileTrap("jump")
	bodyCtx.loop.resolveExitJumps()
}

func parseSource(src string) *Body {
	body, err := parser.ParseString("", src)
	if err != nil {
		panic(fmt.Errorf("error in source %v", err))
	}
	return body
}

func Sal(src string, ast bool) libsam.Blob {
	body := parseSource(src)

	if ast {
		if err := json.NewEncoder(os.Stdout).Encode(body); err != nil {
			panic("error encoding JSON for AST")
		}
	}

	// Wrap the top-level body in a Block and compile it
	block := Block{Pos: body.Pos, Body: body}
	captures := make([]Capture, 0)
	frame := Frame{
		asm: &assembler{array: libsam.NewArray()},
	}
	ctx := Scope{
		frame:     &frame,
		locals:    make([]Local, 0),
		captures:  &captures,
		exitJumps: make([]libsam.Uword, 0),
	}
	ctx.compileBlock(&block)
	ctx.compileInst("halt")

	return ctx.frame.asm.array
}
