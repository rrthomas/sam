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

type Declaration struct {
	Pos lexer.Position

	Variable *string     `@Ident ":"`
	Value    *Expression `@@ ";"`
}

// FIXME: Add optional semi-colon/newline = semicolon
type Statement struct {
	Pos lexer.Position

	Assignment *Assignment `  @@ ";"`
	Return     *Expression `| "return" @@ ";"`
	Expression *Expression `| @@ ";"`
}

type Body struct {
	Pos lexer.Position

	Declarations *[]Declaration `@@*`
	Statements   *[]Statement   `@@*`
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
	for _, c := range *e.Calls {
		out = append(out, c.String())
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

func (d *Declaration) String() string {
	return fmt.Sprintf("%s: float %s", *d.Variable, d.Value.String())
}

func (s *Statement) String() string {
	if s.Expression != nil {
		return s.Expression.String()
	} else if s.Assignment != nil {
		return s.Assignment.String()
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

func (e *PrimaryExp) Compile(ctx *Context) {
	if e.Int != nil {
		ctx.assemble(fmt.Sprintf("int %d", *e.Int))
	} else if e.Float != nil {
		ctx.assemble(fmt.Sprintf("float %g", *e.Float))
	} else if e.Variable != nil {
		ctx.assemble(fmt.Sprintf("int %d", ctx.variableAddr(*e.Variable)), "get")
	} else if e.Block != nil {
		e.Block.Compile(ctx)
	} else if e.Function != nil {
		e.Function.Compile(ctx)
	} else if e.Paren != nil {
		e.Paren.Compile(ctx)
	} else {
		panic("invalid PrimaryExp")
	}
}

func (e *UnaryExp) Compile(ctx *Context) {
	if e.UnaryExp != nil {
		panic("implement compilation of UnaryExp")
	} else if e.PostfixExp != nil {
		e.PostfixExp.Compile(ctx)
	} else {
		panic("invalid UnaryExp")
	}
}

func (e *CallExp) Compile(ctx *Context) {
	// Argument lists
	if e.Calls != nil {
		for i := len(*e.Calls) - 1; i >= 0; i-- {
			(*e.Calls)[i].Compile(ctx)
		}
	}
	// Initial function
	e.Function.Compile(ctx)
	if e.Calls != nil {
		for _, c := range *e.Calls {
			ctx.assemble("do")
			// Pop any arguments, but leave return value
			for i := 0; i < len(*c.Arguments)-1; i++ {
				ctx.assemble("pop")
			}
		}
	}
}

func (a *Args) Compile(ctx *Context) {
	// Push arguments
	for _, a := range *a.Arguments {
		a.Compile(ctx)
	}
	// If there are no arguments, leave space for return value.
	if len(*a.Arguments) == 0 {
		ctx.assemble("int 0")
	}
}

func (e *ExponentExp) Compile(ctx *Context) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		ctx.assemble("pow")
	}
}

func (e *ProductExp) Compile(ctx *Context) {
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

func (e *SumExp) Compile(ctx *Context) {
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

func (e *CompareExp) Compile(ctx *Context) {
	e.Left.Compile(ctx)
	if e.Right != nil {
		e.Right.Compile(ctx)
		switch e.Op {
		case "==":
			ctx.assemble("eq")
		case "!=":
			ctx.assemble("eq", "neg", "not", "neg")
		case "<":
			ctx.assemble("lt")
		case "<=":
			ctx.assemble("-2", "swap", "lt", "neg", "not", "neg")
		case ">":
			ctx.assemble("-2", "swap", "lt")
		case ">=":
			ctx.assemble("lt", "neg", "not", "neg")
		default:
			panic(fmt.Errorf("unknown SumExp.Op %s", e.Op))
		}
	}
}

func (e *BitwiseExp) Compile(ctx *Context) {
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

func (e *LogicNotExp) Compile(ctx *Context) {
	if e.LogicNotExp != nil {
		e.LogicNotExp.Compile(ctx)
		ctx.assemble("neg", "not", "neg")
	} else if e.BitwiseExp != nil {
		e.BitwiseExp.Compile(ctx)
	} else {
		panic("invalid LogicNotExp")
	}

}

func (e *LogicExp) Compile(ctx *Context) {
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

func (e *Expression) Compile(ctx *Context) {
	if e.Cond != nil {
		e.Cond.Compile(ctx)
	} else if e.LogicExp != nil {
		e.LogicExp.Compile(ctx)
	} else {
		panic("invalid Expression")
	}
}

func (c *If) Compile(ctx *Context) {
	ctx.assemble("int 0")
	c.Cond.Compile(ctx)
	c.Then.Compile(ctx)
	if c.Else != nil {
		c.Else.Compile(ctx)
	} else {
		ctx.assemble([]string{})
	}
	ctx.assemble("if")
}

func (a *Assignment) Compile(ctx *Context) {
	a.Expression.Compile(ctx)
	ctx.assemble("int -1", "get") // Duplicate value of expression
	ctx.assemble(fmt.Sprintf("int %d", ctx.variableAddr(*a.Variable)+2), "set")
}

func (d *Declaration) Compile(ctx *Context) {
	ctx.labels = append(ctx.labels, Location{id: *d.Variable, addr: int(ctx.sp)})
	d.Value.Compile(ctx)
}

func (s *Statement) Compile(ctx *Context) {
	if s.Expression != nil {
		s.Expression.Compile(ctx)
	} else if s.Return != nil {
		s.Return.Compile(ctx)
		ctx.setReturn()
		ctx.assemble("int 0", "while")
	} else if s.Assignment != nil {
		s.Assignment.Compile(ctx)
	} else {
		panic("invalid Statement")
	}
}

func (b *Body) Compile(ctx *Context) {
	innerCtx := Context{labels: slices.Clone(ctx.labels), asm: make([]any, 0), sp: 1}
	if b.Declarations != nil {
		for _, d := range *b.Declarations {
			d.Compile(&innerCtx)
		}
	}
	for i, s := range *b.Statements {
		s.Compile(&innerCtx)
		if i < len(*b.Statements)-1 {
			innerCtx.assemble("pop")
		}
	}
	innerCtx.setReturn()
	if b.Declarations != nil {
		for range *b.Declarations {
			innerCtx.assemble("pop")
		}
	}
	ctx.assemble(innerCtx.asm)
}

func (b *Block) Compile(ctx *Context) {
	b.Body.Compile(ctx)
}

func (f *Function) Compile(ctx *Context) {
	// FIXME: captures
	innerCtx := Context{labels: make([]Location, 0), asm: make([]any, 0), sp: 0}
	for i, p := range *f.Parameters {
		innerCtx.labels = append(innerCtx.labels, Location{id: p, addr: i - len(*f.Parameters)})
	}
	f.Body.Compile(&innerCtx)
	ctx.assemble(innerCtx.asm...)
}

type Location struct {
	id   string
	addr int // relative to base of stack frame
}

func (ctx *Context) variableAddr(id string) int {
	for i := len(ctx.labels) - 1; i >= 0; i-- {
		l := ctx.labels[i]
		if l.id == id {
			return int(l.addr) - int(ctx.sp)
		}
	}
	panic(fmt.Errorf("no such variable %s", id))
}

func (ctx *Context) setReturn() {
	ctx.assemble(fmt.Sprintf("int  %d", -int(ctx.sp)), "set")
}

type Context struct {
	labels []Location
	asm    []any
	sp     uint
}

func (ctx *Context) assemble(insts ...any) {
	for _, i := range insts {
		switch inst := i.(type) {
		case string:
			instName := strings.Fields(inst)[0]
			delta, ok := libsam.StackDifference[instName]
			if !ok {
				panic(fmt.Errorf("invalid instruction %s", instName))
			}
			if delta < 0 {
				ctx.sp -= uint(-delta)
			} else {
				ctx.sp += uint(delta)
			}
		default: // Assume an array
			ctx.sp += 1 // A stack pushes a `ref` instruction
		}
		ctx.asm = append(ctx.asm, i)
	}
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
	ctx := Context{labels: make([]Location, 0), asm: make([]any, 0), sp: 0}
	ctx.assemble("int 0")
	body.Compile(&ctx)
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
