//nolint:govet
package main

import (
	"encoding/json"
	"fmt"
	"os"
	"slices"
	"strings"

	"github.com/alecthomas/participle/v2"
	"github.com/alecthomas/participle/v2/lexer"
	"github.com/goccy/go-yaml"
	"github.com/rrthomas/sam/libsam"
)

type PrimaryExp struct {
	Pos lexer.Position

	Int   *int64   `  @Int`
	Float *float64 `| @Float`
	// String
	// List
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

	Cond     *If       `  @@`
	LogicExp *LogicExp `| @@`
}

type If struct {
	Pos lexer.Position

	Cond *Expression `"if" @@`
	Then *Block      `@@`
	Else *Block      `@@?`
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

// FIXME: Add optional semi-colon/newline = semicolon
type Statement struct {
	Pos lexer.Position

	Assignment *Assignment `  @@ ";"`
	Trap       *Trap       `| @@ ";"`
	Expression *Expression `| @@ ";"`
}

type Terminator struct {
	Return *Expression `"return" @@ ";"`
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

// Display

func (e *PrimaryExp) String() string {
	if e.Int != nil {
		return fmt.Sprintf("%d", *e.Int)
	} else if e.Float != nil {
		return fmt.Sprintf("%g", *e.Float)
	} else if e.Variable != nil {
		return *e.Variable
	} else if e.Block != nil {
		return e.Block.String()
	} else if e.Function != nil {
		return e.Function.String()
	} else if e.Paren != nil {
		return "(" + e.Paren.String() + ")"
	}
	panic("invalid PrimaryExp")
}

func (e *CallExp) String() string {
	out := []string{e.Function.String()}
	for _, a := range *e.Calls {
		out = append(out, a.String())
	}
	return strings.Join(out, "")
}

func (a *Args) String() string {
	out := []string{}
	for _, arg := range *a.Arguments {
		out = append(out, arg.String())
	}
	return strings.Join(out, "")
}

func (e *UnaryExp) String() string {
	if e.UnaryExp != nil {
		return fmt.Sprintf("%s%s", e.Op, e.UnaryExp.String())
	} else if e.PostfixExp != nil {
		return e.PostfixExp.String()
	}
	panic("invalid UnaryExp")
}

func (e *ExponentExp) String() string {
	out := []string{e.Left.String()}
	if e.Right != nil {
		out = append(out, "**", e.Right.String())
	}
	return strings.Join(out, " ")
}

func (e *ProductExp) String() string {
	out := []string{e.Left.String()}
	if e.Right != nil {
		out = append(out, e.Op, e.Right.String())
	}
	return strings.Join(out, " ")
}

func (e *SumExp) String() string {
	out := []string{e.Left.String()}
	if e.Right != nil {
		out = append(out, e.Op, e.Right.String())
	}
	return strings.Join(out, " ")
}

func (e *CompareExp) String() string {
	out := []string{e.Left.String()}
	if e.Right != nil {
		out = append(out, e.Op, e.Right.String())
	}
	return strings.Join(out, " ")
}

func (e *BitwiseExp) String() string {
	out := []string{e.Left.String()}
	if e.Right != nil {
		out = append(out, e.Op, e.Right.String())
	}
	return strings.Join(out, " ")
}

func (e *LogicNotExp) String() string {
	if e.LogicNotExp != nil {
		return fmt.Sprintf("not %s", e.LogicNotExp.String())
	} else if e.BitwiseExp != nil {
		return e.BitwiseExp.String()
	}
	panic("invalid LogicNotExp")
}

func (e *LogicExp) String() string {
	out := []string{e.Left.String()}
	if e.Right != nil {
		out = append(out, e.Op, e.Right.String())
	}
	return strings.Join(out, " ")
}

func (e *Expression) String() string {
	if e.Cond != nil {
		return e.Cond.String()
	} else if e.LogicExp != nil {
		return e.LogicExp.String()
	}
	panic("invalid Expression")
}

func (c *If) String() string {
	out := []string{fmt.Sprintf("if %s %s", c.Cond.String(), c.Then.String())}
	if c.Else != nil {
		out = append(out, c.Else.String())
	}
	return strings.Join(out, " ")
}

func (a *Assignment) String() string {
	return fmt.Sprintf("%s = %s", *a.Variable, a.Expression.String())
}

func (t *Trap) String() string {
	out := []string{fmt.Sprintf("trap %s", *t.Function)}
	for _, a := range *t.Arguments {
		out = append(out, a.String())
	}
	return strings.Join(out, ",")
}

func (d *Declaration) String() string {
	return fmt.Sprintf("%s: float %s", *d.Variable, d.Value.String())
}

func (s *Statement) String() string {
	if s.Expression != nil {
		return s.Expression.String()
	} else if s.Assignment != nil {
		return s.Assignment.String()
	} else if s.Trap != nil {
		return s.Trap.String()
	}
	panic("invalid statement")
}

func (t *Terminator) String() string {
	if t.Return != nil {
		return fmt.Sprintf("return %s", t.Return.String())
	}
	panic("invalid statement")
}

func (b *Body) String() string {
	out := []string{}
	for _, d := range *b.Declarations {
		out = append(out, d.String())
	}
	for _, s := range *b.Statements {
		out = append(out, s.String())
	}
	return strings.Join(out, "\n")
}

func (b *Block) String() string {
	// FIXME: Indentation
	return fmt.Sprintf("{\n%s\n}", b.Body.String())
}

func (f *Function) String() string {
	header := []string{"fn", "("}
	header = append(header, *f.Parameters...)
	header = append(header, ")")
	out := []string{strings.Join(header, " ")}
	out = append(out, f.Body.String())
	return strings.Join(out, "\n")
}

// Compilation

func (e *PrimaryExp) Compile(ctx *Frame) {
	if e.Int != nil {
		ctx.assemble(fmt.Sprintf("int %d", *e.Int))
	} else if e.Float != nil {
		ctx.assemble(fmt.Sprintf("float %g", *e.Float))
	} else if e.Variable != nil {
		ctx.assemble(fmt.Sprintf("int %d", ctx.variableAddr(*e.Variable)), "get")
	} else if e.Block != nil {
		ctx.assemble("int 0")
		blockCtx := e.Block.Compile(ctx)
		ctx.assemble(blockCtx.asm)
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
				ctx.sp -= libsam.Uword(nargs - 1)
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
			ctx.assemble("lsh")
		case ">>":
			ctx.assemble("rsh")
		case ">>>":
			ctx.assemble("arsh")
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
	if e.Cond != nil {
		e.Cond.Compile(ctx)
	} else if e.LogicExp != nil {
		e.LogicExp.Compile(ctx)
	} else {
		panic("invalid Expression")
	}
}

func (c *If) Compile(ctx *Frame) {
	ctx.assemble("int 0") // return value
	thenCtx := c.Then.Compile(ctx)
	var elseCtx Frame = Frame{}
	if c.Else != nil {
		elseCtx = c.Else.Compile(ctx)
	}
	c.Cond.Compile(ctx)
	ctx.assemble(thenCtx.asm)
	ctx.assemble(elseCtx.asm)
	ctx.assemble("if")
}

func (a *Assignment) Compile(ctx *Frame) {
	a.Expression.Compile(ctx)
	ctx.assemble("_one", "get", fmt.Sprintf("int %d", ctx.variableAddr(*a.Variable)), "set")
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
		ctx.returnFromFrame = true
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
				ctx.assemblePop(1)
			}
		}
	}
	if b.Terminator != nil {
		b.Terminator.Compile(ctx)
	}
}

func (b *Block) Compile(ctx *Frame) Frame {
	baseSp := libsam.Uword(int(ctx.sp) + 1)
	blockCtx := Frame{
		labels:          slices.Clone(ctx.labels),
		asm:             make([]any, 0),
		sp:              baseSp,
		nargs:           ctx.nargs,
		returnFromFrame: ctx.returnFromFrame,
	}
	b.Body.Compile(&blockCtx)
	if blockCtx.returnFromFrame {
		blockCtx.tearDownFrame()
	} else {
		blockCtx.tearDownBlock(blockCtx.sp - baseSp)
	}
	return blockCtx
}

func (f *Function) Compile(ctx *Frame) {
	// FIXME: captures
	nargs := libsam.Uword(len(*f.Parameters))
	innerCtx := Frame{labels: make([]Location, 0), asm: make([]any, 0), sp: 0, nargs: nargs}
	for i, p := range *f.Parameters {
		innerCtx.labels = append(innerCtx.labels, Location{id: p, addr: i - int(nargs)})
	}
	innerCtx.returnFromFrame = true
	ctx.assemble(f.Body.Compile(&innerCtx).asm)
}

type Location struct {
	id   string
	addr int // relative to base of stack frame
}

func (ctx *Frame) variableAddr(id string) int {
	for i := len(ctx.labels) - 1; i >= 0; i-- {
		l := ctx.labels[i]
		if l.id == id {
			return int(l.addr) - int(ctx.sp)
		}
	}
	panic(fmt.Errorf("no such variable %s", id))
}

func (ctx *Frame) tearDownBlock(depth libsam.Uword) {
	// Set result
	ctx.assemble(fmt.Sprintf("int %d", -int(depth+1)), "set")
	// Pop remaining stack items in this frame, except for return address
	if depth > 1 {
		ctx.assemblePop(depth - 1)
	}
}

func (ctx *Frame) tearDownFrame() {
	// Set result
	ctx.assemble(fmt.Sprintf("int %d", -int(ctx.sp-1+ctx.nargs)), "set")
	// Pop remaining stack items in this frame, except for return address
	if ctx.sp > 1 {
		ctx.assemblePop(ctx.sp - 1)
	}
	// If we have some arguments still on the stack, need to get rid of them
	if ctx.nargs > 1 {
		// Store the return `pc` over the second argument
		ctx.assemble(fmt.Sprintf("int %d", -int(ctx.nargs)+1), "set")
		if ctx.nargs > 2 {
			// Pop arguments 3 onwards, if any
			ctx.assemblePop(ctx.nargs - 2)
		}
	}
}

type Frame struct {
	labels          []Location
	asm             []any
	sp              libsam.Uword
	nargs           libsam.Uword // Number of arguments to this frame
	returnFromFrame bool         // Whether we return from the frame on block exit
}

func (ctx *Frame) assemble(insts ...any) {
	for _, i := range insts {
		switch inst := i.(type) {
		case string:
			instName := strings.Fields(inst)[0]
			if instName == "pop" {
				panic("use assemblePop to assemble a pop instruction")
			} else if instName == "trap" {
				panic("use assembleTrap to assemble a trap instruction")
			}
			delta, ok := libsam.StackDifference[instName]
			if !ok {
				panic(fmt.Errorf("invalid instruction %s", instName))
			}
			if delta < 0 {
				ctx.sp -= libsam.Uword(-delta)
			} else {
				ctx.sp += libsam.Uword(delta)
			}
		default: // Assume an array
			ctx.sp += 1 // A stack pushes a `ref` instruction
		}
		ctx.asm = append(ctx.asm, i)
	}
}

func (ctx *Frame) assemblePop(items libsam.Uword) {
	ctx.asm = append(ctx.asm, fmt.Sprintf("int %d", items), "pop")
	ctx.sp -= items
}

func trapStackEffect(function string) libsam.StackEffect {
	stackEffect, ok := libsam.TrapStackEffect[function]
	if !ok {
		panic(fmt.Errorf("unknown trap %s", function))
	}
	return stackEffect
}

func (ctx *Frame) assembleTrap(function string) {
	ctx.asm = append(ctx.asm, fmt.Sprintf("trap %s", function))
	stackEffect := trapStackEffect(function)
	ctx.sp -= stackEffect.In
	ctx.sp += stackEffect.Out
}

var parser = participle.MustBuild[Body]()

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
	ctx := Frame{labels: make([]Location, 0), asm: make([]any, 0), sp: 0, nargs: 0}
	ctx.assemble("int 0") // return value
	ctx.assemble(block.Compile(&ctx).asm)
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
